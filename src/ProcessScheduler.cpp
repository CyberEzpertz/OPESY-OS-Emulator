#include "ProcessScheduler.h"

#include <chrono>
#include <print>
#include <thread>

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

    // Wake all threads
    tickCv.notify_all();

    if (tickThread.joinable()) {
        tickThread.join();
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
    tickCv.notify_all();
    readyCv.notify_all();
}

int ProcessScheduler::getNumAvailableCores() const {
    return availableCores.load();
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
        std::lock_guard<std::mutex> lock(readyMutex);
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
    return cpuCycles.load();
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

void ProcessScheduler::workerLoop(const int coreId) {
    uint64_t lastTickSeen = 0;
    std::shared_ptr<Process> proc = nullptr;

    while (running) {
        // Get a process from the queue
        {
            std::unique_lock<std::mutex> lock(readyMutex);

            readyCv.wait(lock, [&] { return !readyQueue.empty() || !running; });

            if (!readyQueue.empty()) {
                proc = readyQueue.front();
                readyQueue.pop_front();

                proc->setStatus(RUNNING);
                proc->setCurrentCore(coreId);
                availableCores -= 1;
            }
        }

        // NOTE: This is only for FCFS, RR will be implemented in the future
        while (proc && proc->getStatus() == RUNNING) {
            // Wait for next tick before executing next instruction
            {
                std::unique_lock<std::mutex> lock(tickMutex);
                tickCv.wait(
                    lock, [&] { return cpuCycles > lastTickSeen || !running; });

                if (!running)
                    break;
                lastTickSeen = cpuCycles;
            }

            proc->incrementLine();
        }

        // Reset current core to none
        if (proc) {
            proc->setCurrentCore(-1);
            proc = nullptr;
            availableCores += 1;
        }
    }
}