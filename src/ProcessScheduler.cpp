#include "ProcessScheduler.h"

#include <chrono>
#include <thread>

#include "Process.h"

using namespace std::chrono_literals;

ProcessScheduler& ProcessScheduler::getInstance() {
    static ProcessScheduler instance;
    return instance;
}

ProcessScheduler::ProcessScheduler() = default;

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
    queueCv.notify_all();
}

int ProcessScheduler::getNumAvailableCores() const {
    return availableCores.load();
}

int ProcessScheduler::getNumTotalCores() const {
    return numCpuCores;
}

void ProcessScheduler::initialize(const int numCores) {
    this->numCpuCores = numCores;
    this->availableCores = numCores;
}

void ProcessScheduler::scheduleProcess(
    const std::shared_ptr<Process>& process) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        processQueue.push_back(process);
    }

    // Wake one worker in case they were waiting for work
    queueCv.notify_one();
}

void ProcessScheduler::sortQueue() {
    // TODO: Implement this when needed
}

uint64_t ProcessScheduler::getCurrentCycle() const {
    return cpuCycles.load();
}

void ProcessScheduler::incrementCpuCycles() {
    {
        std::lock_guard<std::mutex> lock(tickMutex);
        ++cpuCycles;

        // if (processQueue.empty() && availableCores.load() == numCpuCores) {
        //     running = false;
        // }
    }
    tickCv.notify_all();  // Notify all worker threads that a new tick occurred
}

void ProcessScheduler::tickLoop() {
    while (running) {
        std::this_thread::sleep_for(10ms);  // Simulate one tick every 50ms
        incrementCpuCycles();
    }
}

void ProcessScheduler::workerLoop(int coreId) {
    uint64_t lastTickSeen = 0;
    std::shared_ptr<Process> proc = nullptr;

    while (running) {
        // Get a process from the queue
        {
            std::unique_lock<std::mutex> lock(queueMutex);

            queueCv.wait(lock,
                         [&] { return !processQueue.empty() || !running; });

            if (!processQueue.empty()) {
                proc = processQueue.front();
                processQueue.pop_front();

                proc->setStatus(RUNNING);
                proc->setCurrentCore(coreId);
                availableCores -= 1;
            }
        }

        // NOTE: This is only for FCFS, RR will be implemented in the future
        while (proc && proc->getStatus() != DONE) {
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