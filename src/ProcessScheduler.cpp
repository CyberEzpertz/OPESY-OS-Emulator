#include "ProcessScheduler.h"

#include <chrono>
#include <print>
#include <thread>

#include "ConsoleManager.h"
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

void ProcessScheduler::scheduleProcess(
    const std::shared_ptr<Process>& process) {
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

        while (!waitQueue.empty() &&
               waitQueue.top()->getWakeupTick() <= cpuCycles) {
            auto proc = waitQueue.top();
            waitQueue.pop();

            if (!proc)
                continue;  // skip nulls

            if (proc->getIsFinished()) {
                proc->setStatus(DONE);
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
    dummyGeneratorThread =
        std::thread(&ProcessScheduler::dummyGeneratorLoop, this);
    std::println("Started dummy process generation every {} CPU cycles.",
                 Config::getInstance().getBatchProcessFreq());
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
    const int interval = Config::getInstance().getBatchProcessFreq();
    uint64_t lastCycle = getCurrentCycle();

    while (generatingDummies) {
        // wait until either: (a) we’re told to stop, or (b) enough ticks passed
        {
            std::unique_lock lock(tickMutex);
            tickCv.wait(lock, [&] {
                return !generatingDummies ||
                       (getCurrentCycle() - lastCycle) >= interval;
            });
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
        // std::this_thread::sleep_for(1ms);  // Simulate one tick every 50ms
        incrementCpuCycles();
    }
}

void ProcessScheduler::executeFCFS(std::shared_ptr<Process>& proc,
                                   uint64_t& lastTickSeen) {
    // FCFS: Run until process is finished or blocked
    const uint32_t delayCycles = Config::getInstance().getDelaysPerExec();
    while (proc && proc->getStatus() == RUNNING) {
        // Wait for next tick before executing next instruction
        {
            std::unique_lock lock(tickMutex);
            tickCv.wait(lock,
                        [&] { return cpuCycles > lastTickSeen || !running; });

            if (!running)
                break;
            lastTickSeen = cpuCycles;
        }

        if (delayCycles == 0 || cpuCycles % delayCycles == 0) {
            proc->incrementLine();
        }
    }
}

void ProcessScheduler::executeRR(std::shared_ptr<Process>& proc,
                                 uint64_t& lastTickSeen) {
    // Round Robin: Run for quantum cycles or until finished/blocked
    uint32_t cyclesExecuted = 0;
    const uint32_t delayCycles   = Config::getInstance().getDelaysPerExec();
    const auto quantumCycles = Config::getInstance().getQuantumCycles();
    while (proc && proc->getStatus() == RUNNING &&
           cyclesExecuted < quantumCycles) {
        // Wait for next tick before executing next instruction
        {
            std::unique_lock lock(tickMutex);
            tickCv.wait(lock,
                        [&] { return cpuCycles > lastTickSeen || !running; });

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
    if (proc && proc->getStatus() == RUNNING &&
        cyclesExecuted >= quantumCycles) {
        proc->setStatus(READY);
        scheduleProcess(proc);  // Put back at end of ready queue
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

                proc->setStatus(RUNNING);
                proc->setCurrentCore(coreId);
                availableCores -= 1;
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

        // Reset current core to none
        if (proc) {
            proc->setCurrentCore(-1);
            proc = nullptr;
            availableCores += 1;
        }
    }
}