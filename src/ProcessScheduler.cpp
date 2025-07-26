#include "ProcessScheduler.h"

#include <chrono>
#include <print>
#include <thread>

#include "ConsoleManager.h"
#include "FlatMemoryAllocator.h"  // Add this include
#include "PagingAllocator.h"
#include "Process.h"

using namespace std::chrono_literals;

ProcessScheduler& ProcessScheduler::getInstance() {
    static auto* instance = new ProcessScheduler();
    return *instance;
}

ProcessScheduler::ProcessScheduler() {
    this->numCpuCores = Config::getInstance().getNumCPUs();
    this->availableCores = Config::getInstance().getNumCPUs();
};

ProcessScheduler::~ProcessScheduler() {
    running = false;
    generatingDummies = false;

    // Wake all threads
    tickCv.notify_all();

    if (tickThread.joinable()) {
        tickThread.join();
    }

    if (dummyGeneratorThread.joinable()) {
        dummyGeneratorThread.join();
    }

    for (auto& t : cpuWorkers) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void ProcessScheduler::start() {
    running = true;

    // Start CPU worker threads
    for (int i = 0; i < numCpuCores; ++i) {
        cpuWorkers.emplace_back(&ProcessScheduler::workerLoop, this, i);
    }

    // Start ticking thread
    tickThread = std::thread(&ProcessScheduler::tickLoop, this);
}

void ProcessScheduler::stop() {
    running = false;
    generatingDummies = false;
    tickCv.notify_all();
}

int ProcessScheduler::getNumAvailableCores() const {
    return availableCores;
}

int ProcessScheduler::getNumTotalCores() const {
    return numCpuCores;
}

void ProcessScheduler::initialize() {
    this->numCpuCores = Config::getInstance().getNumCPUs();
    this->availableCores = Config::getInstance().getNumCPUs();
}

void ProcessScheduler::scheduleProcess(const std::shared_ptr<Process>& process) {
    {
        std::lock_guard lock(readyMutex);
        readyQueue.push_back(process);
    }
}

void ProcessScheduler::sleepProcess(const std::shared_ptr<Process>& process) {
    {
        std::lock_guard lock(waitMutex);
        this->waitQueue.push(process);
    }
}

uint64_t ProcessScheduler::getCurrentCycle() const {
    return totalCPUTicks;
}

void ProcessScheduler::incrementCpuTicks() {
    {
        std::unique_lock lock(tickMutex);

        // Only tick when all cores are finished executing an instruction
        tickCv.wait(lock, [&] { return coresFinishedThisTick >= numCpuCores || totalCPUTicks == 0; });
    }

    // Wakeup all sleeping processes that need to wakeup
    {
        std::lock_guard lock(waitMutex);

        while (!waitQueue.empty() && waitQueue.top()->getWakeupTick() <= (totalCPUTicks + 1)) {
            auto proc = waitQueue.top();
            waitQueue.pop();

            if (!proc)
                continue;  // skip nulls

            if (proc->getIsFinished()) {
                proc->setStatus(DONE);
                // Deallocate memory for finished process
                deallocateProcessMemory(proc);
            } else {
                proc->setStatus(READY);
                scheduleProcess(proc);
            }
        }
    }

    ++totalCPUTicks;
    coresFinishedThisTick = 0;
    tickCv.notify_all();  // Notify all worker threads that a new tick occurred
}

void ProcessScheduler::startDummyGeneration() {
    if (generatingDummies) {
        std::println("Dummy process generation is already running.");
        return;
    }

    generatingDummies = true;
    dummyGeneratorThread = std::thread(&ProcessScheduler::dummyGeneratorLoop, this);
    std::println("Started dummy process generation every {} CPU cycles.", Config::getInstance().getBatchProcessFreq());
}

void ProcessScheduler::stopDummyGeneration() {
    if (!generatingDummies.exchange(false)) {
        std::println("Dummy process generation is not currently running.");
        return;
    }
    tickCv.notify_all();  // wake the loop above

    if (dummyGeneratorThread.joinable())
        dummyGeneratorThread.join();

    std::println("Stopped dummy process generation.");
}

void ProcessScheduler::dummyGeneratorLoop() {
    const auto interval = Config::getInstance().getBatchProcessFreq();
    uint64_t lastCycle = getCurrentCycle();

    while (generatingDummies) {
        // wait until either: (a) we're told to stop, or (b) enough ticks passed
        {
            std::unique_lock lock(tickMutex);
            tickCv.wait(lock, [&] { return !generatingDummies || (getCurrentCycle() - lastCycle) >= interval; });
        }

        if (!generatingDummies)
            break;

        lastCycle = getCurrentCycle();  // time for a new batch!

        int id = ConsoleManager::getInstance().getProcessIdList().size();
        std::string name = std::format("process_{:02d}", id);

        ConsoleManager::getInstance().createDummyProcess(name);
    }
}

bool ProcessScheduler::isGeneratingDummies() const {
    return generatingDummies;
}

void ProcessScheduler::printQueues() const {
    std::println("Ready queue: {}", this->readyQueue.size());
    std::println("Waiting queue: {}", this->waitQueue.size());
}

void ProcessScheduler::tickLoop() {
    while (running) {
        std::this_thread::sleep_for(1ms);  // Simulate one tick every 1ms
        incrementCpuTicks();
    }
}

void ProcessScheduler::executeFCFS(const std::shared_ptr<Process>& proc, uint64_t& lastTickSeen) {
    // FCFS: Run until process is finished or blocked
    const uint32_t delayCycles = Config::getInstance().getDelaysPerExec();
    while (proc && proc->getStatus() == RUNNING) {
        // Wait for next tick before executing next instruction
        {
            std::unique_lock lock(tickMutex);
            tickCv.wait(lock, [&] { return totalCPUTicks > lastTickSeen || !running; });

            if (!running)
                break;
            lastTickSeen = totalCPUTicks;
        }

        if (delayCycles == 0 || totalCPUTicks % delayCycles == 0) {
            proc->incrementLine();
        }

        ++activeCpuTicks;
        ++coresFinishedThisTick;

        if (coresFinishedThisTick >= numCpuCores) {
            tickCv.notify_all();  // only notify when all cores are done
        }
    }
}

void ProcessScheduler::executeRR(const std::shared_ptr<Process>& proc, uint64_t& lastTickSeen) {
    // Round Robin: Run for quantum cycles or until finished/blocked
    uint32_t cyclesExecuted = 0;
    const uint32_t delayCycles = Config::getInstance().getDelaysPerExec();
    const auto quantumCycles = Config::getInstance().getQuantumCycles();

    while (proc && proc->getStatus() == RUNNING && cyclesExecuted < quantumCycles) {
        // Wait for next tick before executing next instruction
        {
            std::unique_lock lock(tickMutex);
            tickCv.wait(lock, [&] { return totalCPUTicks > lastTickSeen || !running; });

            if (!running)
                break;
            lastTickSeen = totalCPUTicks;
        }

        if (delayCycles == 0 || totalCPUTicks % delayCycles == 0) {
            proc->incrementLine();
            cyclesExecuted++;
        }

        ++activeCpuTicks;
        ++coresFinishedThisTick;

        if (coresFinishedThisTick >= numCpuCores) {
            tickCv.notify_all();  // only notify when all cores are done
        }
    }

    // If process used full quantum and is still running, preempt it
    if (proc && proc->getStatus() == RUNNING && cyclesExecuted >= quantumCycles) {
        proc->setStatus(READY);
        scheduleProcess(proc);  // Put back at end of ready queue
    }
}

void ProcessScheduler::resetCore(std::shared_ptr<Process>& proc) {
    // Check if process is finished
    if (proc->getIsFinished()) {
        proc->setStatus(DONE);
        // Deallocate memory for completed process
        PagingAllocator::getInstance().deallocate(proc->getID());
    }

    // Reset current core to none
    proc->setCurrentCore(-1);
    proc = nullptr;
    availableCores += 1;
}

void ProcessScheduler::workerLoop(const int coreId) {
    uint64_t lastTickSeen = 0;
    std::shared_ptr<Process> proc = nullptr;
    const auto schedulerType = Config::getInstance().getSchedulerType();

    while (running) {
        lastTickSeen = totalCPUTicks;

        // Wait for tick to be updated
        {
            std::unique_lock lock(tickMutex);
            tickCv.wait(lock, [&] { return totalCPUTicks > lastTickSeen || !running; });

            if (!running)
                break;
        }

        // Get a process from the queue
        {
            std::lock_guard lock(readyMutex);

            if (!readyQueue.empty()) {
                proc = readyQueue.front();
                readyQueue.pop_front();

                proc->setStatus(RUNNING);
                proc->setCurrentCore(coreId);
                availableCores -= 1;
            }
        }

        // Idle if core wasn't able to find a process
        if (!proc) {
            idleCpuTicks += 1;
            coresFinishedThisTick += 1;
            tickCv.notify_all();

            continue;
        }

        // Execute process based on scheduler type
        if (schedulerType == SchedulerType::FCFS) {
            executeFCFS(proc, lastTickSeen);
        } else {
            executeRR(proc, lastTickSeen);
        }

        resetCore(proc);
    }
}

// DEPRECATED: This is for Flat Memory Allocator only
bool ProcessScheduler::tryAllocateMemory(std::shared_ptr<Process>& proc) {
    // Check if process already has memory allocated
    if (proc->getBaseAddress() != nullptr) {
        return true;  // Memory already allocated
    }

    // Attempt to allocate memory
    const void* allocatedMemory = FlatMemoryAllocator::getInstance().allocate(proc->getRequiredMemory(), proc);

    if (allocatedMemory == nullptr) {
        // Memory allocation failed
        // For debugging
        // std::println("Process {} failed to allocate {} bytes of memory", proc->getName(), proc->getRequiredMemory());
    } else {
        // Memory allocation successful
        // For debugging
        // std::println("Process {} allocated {} bytes of memory at address {}", proc->getName(),
        // proc->getRequiredMemory(), allocatedMemory);
    }

    return allocatedMemory != nullptr;
}

// DEPRECATED: This is for Flat Memory Allocator only
void ProcessScheduler::deallocateProcessMemory(const std::shared_ptr<Process>& proc) const {
    if (proc->getBaseAddress() != nullptr) {
        FlatMemoryAllocator::getInstance().deallocate(proc->getBaseAddress(), proc);

        // For debugging
        // std::println("Process {} deallocated memory at address {}", proc->getName(), proc->getBaseAddress());
    }
}