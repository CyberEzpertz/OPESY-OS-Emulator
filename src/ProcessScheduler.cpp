#include "ProcessScheduler.h"

#include <chrono>
#include <functional>
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

    if (tickThread.joinable())
        tickThread.join();

    if (dummyGeneratorThread.joinable())
        dummyGeneratorThread.join();

    for (auto& t : cpuWorkers) {
        if (t.joinable())
            t.join();
    }

    tickBarrier = nullptr;
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
}

int ProcessScheduler::getNumAvailableCores() const {
    return availableCores;
}

int ProcessScheduler::getNumTotalCores() const {
    return numCpuCores;
}

uint64_t ProcessScheduler::getIdleCPUTicks() const {
    return idleCpuTicks;
}

uint64_t ProcessScheduler::getActiveCPUTicks() const {
    return activeCpuTicks;
}

std::vector<std::shared_ptr<Process>> ProcessScheduler::getCoreAssignments() const {
    std::lock_guard lock(coreAssignmentsMutex);

    return coreAssignments;
}

void ProcessScheduler::initialize() {
    this->numCpuCores = Config::getInstance().getNumCPUs();
    this->availableCores = Config::getInstance().getNumCPUs();
    coreAssignments.resize(numCpuCores);  // One slot per core

    tickBarrier =
        std::make_unique<std::barrier<std::function<void()>>>(numCpuCores + 1, [this] { incrementCpuTicks(); });
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

uint64_t ProcessScheduler::getTotalCPUTicks() const {
    return totalCPUTicks;
}

void ProcessScheduler::incrementCpuTicks() {
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
                PagingAllocator::getInstance().deallocate(proc->getID());
            } else {
                proc->setStatus(READY);
                scheduleProcess(proc);
            }
        }
    }

    tickCv.notify_all();
    ++totalCPUTicks;
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

    // Notify the waiting thread
    tickCv.notify_all();

    if (dummyGeneratorThread.joinable())
        dummyGeneratorThread.join();

    std::println("Stopped dummy process generation.");
}

void ProcessScheduler::dummyGeneratorLoop() {
    const auto interval = Config::getInstance().getBatchProcessFreq();
    uint64_t lastCycle = getTotalCPUTicks();

    while (generatingDummies) {
        // wait until either: (a) we're told to stop, or (b) enough ticks passed
        {
            std::unique_lock lock(tickMutex);
            tickCv.wait(lock, [&] { return !generatingDummies || (getTotalCPUTicks() - lastCycle) >= interval; });
        }

        if (!generatingDummies)
            break;

        lastCycle = getTotalCPUTicks();  // time for a new batch!

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
        tickBarrier->arrive_and_wait();
    }
    tickBarrier->arrive_and_drop();
}

void ProcessScheduler::executeFCFS(const std::shared_ptr<Process>& proc, uint64_t& lastTickSeen) {
    // FCFS: Run until process is finished or blocked
    const uint32_t delayCycles = Config::getInstance().getDelaysPerExec();
    while (proc && proc->getStatus() == RUNNING) {
        if (!running)
            break;

        lastTickSeen = totalCPUTicks;

        if (delayCycles == 0 || totalCPUTicks % delayCycles == 0) {
            proc->incrementLine();
        }

        ++activeCpuTicks;
        tickBarrier->arrive_and_wait();
    }
}

void ProcessScheduler::executeRR(const std::shared_ptr<Process>& proc, uint64_t& lastTickSeen) {
    // Round Robin: Run for quantum cycles or until finished/blocked
    uint32_t cyclesExecuted = 0;
    const uint32_t delayCycles = Config::getInstance().getDelaysPerExec();
    const auto quantumCycles = Config::getInstance().getQuantumCycles();

    while (proc && proc->getStatus() == RUNNING && cyclesExecuted < quantumCycles) {
        if (!running)
            break;
        lastTickSeen = totalCPUTicks;

        if (delayCycles == 0 || totalCPUTicks % delayCycles == 0) {
            proc->incrementLine();
            cyclesExecuted++;
        }

        ++activeCpuTicks;
        tickBarrier->arrive_and_wait();
    }

    // If process used full quantum and is still running, preempt it
    if (proc && proc->getStatus() == RUNNING && cyclesExecuted >= quantumCycles) {
        proc->setStatus(READY);
        scheduleProcess(proc);  // Put back at end of ready queue
    }
}

void ProcessScheduler::resetCore(std::shared_ptr<Process>& proc, int coreId) {
    std::lock_guard lock(coreAssignmentsMutex);
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
    coreAssignments[coreId] = nullptr;  // Clear assignment
}

void ProcessScheduler::workerLoop(const int coreId) {
    uint64_t lastTickSeen = 0;
    std::shared_ptr<Process> proc = nullptr;
    const auto schedulerType = Config::getInstance().getSchedulerType();

    while (running) {
        lastTickSeen = totalCPUTicks;

        // Get a process from the queue
        {
            std::lock_guard lock(readyMutex);

            if (!readyQueue.empty()) {
                proc = readyQueue.front();
                readyQueue.pop_front();

                proc->setStatus(RUNNING);
                proc->setCurrentCore(coreId);
                {
                    std::lock_guard lock2(coreAssignmentsMutex);
                    availableCores -= 1;
                    coreAssignments[coreId] = proc;
                }
            }
        }

        if (!proc) {
            idleCpuTicks += 1;
            tickBarrier->arrive_and_wait();
            continue;
        }

        // Execute process based on scheduler type
        if (schedulerType == SchedulerType::FCFS) {
            executeFCFS(proc, lastTickSeen);
        } else {
            executeRR(proc, lastTickSeen);
        }

        resetCore(proc, coreId);
    }

    tickBarrier->arrive_and_drop();
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