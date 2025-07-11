#include "ProcessScheduler.h"

#include <chrono>
#include <print>
#include <thread>

#include "ConsoleManager.h"
#include "FlatMemoryAllocator.h"  // Add this include
#include "Process.h"

using namespace std::chrono_literals;

ProcessScheduler& ProcessScheduler::getInstance() {
    static ProcessScheduler instance;
    return instance;
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
    readyCv.notify_all();
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

    // Wake one worker in case they were waiting for work
    readyCv.notify_one();
}

void ProcessScheduler::sortQueue() {
    // TODO: Implement this when needed
}

void ProcessScheduler::sleepProcess(const std::shared_ptr<Process>& process) {
    {
        std::lock_guard lock(waitMutex);
        this->waitQueue.push(process);
    }
}

uint64_t ProcessScheduler::getCurrentCycle() const {
    return cpuCycles;
}

void ProcessScheduler::incrementCpuCycles() {
    {
        std::lock_guard lock(tickMutex);
        ++cpuCycles;
    }

    // Wakeup all sleeping processes that need to wakeup
    // NOTE: This should be put before tickCv, otherwise the cores might not be
    // able to get the recently woken up processes
    {
        std::lock_guard lock(waitMutex);

        while (!waitQueue.empty() && waitQueue.top()->getWakeupTick() <= cpuCycles) {
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
        if ((getCurrentCycle() - lastCycle) < interval)
            continue;

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
        std::this_thread::sleep_for(100ms);  // Simulate one tick every 50ms
        incrementCpuCycles();
    }
}

void ProcessScheduler::executeFCFS(const std::shared_ptr<Process>& proc, uint64_t& lastTickSeen) {
    // FCFS: Run until process is finished or blocked
    const uint32_t delayCycles = Config::getInstance().getDelaysPerExec();
    while (proc && proc->getStatus() == RUNNING) {
        // Wait for next tick before executing next instruction
        {
            std::unique_lock lock(tickMutex);
            tickCv.wait(lock, [&] { return cpuCycles > lastTickSeen || !running; });

            if (!running)
                break;
            lastTickSeen = cpuCycles;
        }

        if (delayCycles == 0 || cpuCycles % delayCycles == 0) {
            proc->incrementLine();
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
            tickCv.wait(lock, [&] { return cpuCycles > lastTickSeen || !running; });

            if (!running)
                break;
            lastTickSeen = cpuCycles;
        }

        if (delayCycles == 0 || cpuCycles % delayCycles == 0) {
            proc->incrementLine();
            cyclesExecuted++;
        }
    }

    // If process used full quantum and is still running, preempt it
    if (proc && proc->getStatus() == RUNNING && cyclesExecuted >= quantumCycles) {
        proc->setStatus(READY);
        scheduleProcess(proc);  // Put back at end of ready queue
        // Note: Memory remains allocated during preemption
    }
}

// New method to attempt memory allocation for a process
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

// New method to deallocate memory for a process
void ProcessScheduler::deallocateProcessMemory(const std::shared_ptr<Process>& proc) const {
    if (proc->getBaseAddress() != nullptr) {
        FlatMemoryAllocator::getInstance().deallocate(proc->getBaseAddress(), proc);

        // For debugging
        // std::println("Process {} deallocated memory at address {}", proc->getName(), proc->getBaseAddress());
    }
}

void ProcessScheduler::workerLoop(const int coreId) {
    uint64_t lastTickSeen = 0;
    std::shared_ptr<Process> proc = nullptr;
    const auto schedulerType = Config::getInstance().getSchedulerType();

    while (running) {
        // Get a process from the queue
        {
            std::unique_lock lock(readyMutex);

            readyCv.wait(lock, [&] { return !readyQueue.empty() || !running; });

            if (!running)
                break;

            if (!readyQueue.empty()) {
                proc = readyQueue.front();
                readyQueue.pop_front();

                // CRITICAL: Attempt memory allocation before running the process
                if (tryAllocateMemory(proc)) {
                    // Memory allocation successful - proceed with execution
                    proc->setStatus(RUNNING);
                    proc->setCurrentCore(coreId);
                    availableCores -= 1;
                } else {
                    // Memory allocation failed - move process to back of ready queue
                    readyQueue.push_back(proc);
                    proc = nullptr;  // Don't execute this process
                    continue;        // Try next process in queue
                }
            }
        }

        if (!proc)
            continue;

        // Execute process based on scheduler type
        if (schedulerType == SchedulerType::FCFS) {
            executeFCFS(proc, lastTickSeen);
        } else if (schedulerType == SchedulerType::RR) {
            executeRR(proc, lastTickSeen);
        }

        // Handle process completion or cleanup
        if (proc) {
            // Check if process is finished
            if (proc->getIsFinished()) {
                proc->setStatus(DONE);
                // Deallocate memory for completed process
                deallocateProcessMemory(proc);
            }

            // Reset current core to none
            proc->setCurrentCore(-1);
            proc = nullptr;
            availableCores += 1;
        }
    }
}