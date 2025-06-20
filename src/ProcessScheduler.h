#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "Config.h"
#include "Process.h"

// For the min-heap waiting queue
struct WakeupComparator {
    bool operator()(const std::shared_ptr<Process>& a,
                    const std::shared_ptr<Process>& b) const {
        return a->getWakeupTick() > b->getWakeupTick();
    }
};

class ProcessScheduler {
public:
    static ProcessScheduler& getInstance();

    void start();
    void stop();
    int getNumAvailableCores() const;
    int getNumTotalCores() const;
    void initialize();
    void scheduleProcess(const std::shared_ptr<Process>& process);
    void sortQueue();
    void sleepProcess(const std::shared_ptr<Process>& process);
    uint64_t getCurrentCycle() const;
    void printQueues() const;
    void startDummyGeneration();
    void stopDummyGeneration();
    bool isGeneratingDummies() const;

private:
    ProcessScheduler();
    ~ProcessScheduler();

    ProcessScheduler(const ProcessScheduler&) = delete;
    ProcessScheduler& operator=(const ProcessScheduler&) = delete;

    void tickLoop();
    void workerLoop(int coreId);
    void incrementCpuCycles();
    void dummyGeneratorLoop();
    void executeFCFS(std::shared_ptr<Process>& proc, uint64_t& lastTickSeen);
    void executeRR(std::shared_ptr<Process>& proc, uint64_t& lastTickSeen);

    int numCpuCores;
    std::atomic<int> availableCores;

    std::deque<std::shared_ptr<Process>> readyQueue;
    std::condition_variable readyCv;
    std::mutex readyMutex;

    std::priority_queue<std::shared_ptr<Process>,
                        std::vector<std::shared_ptr<Process>>, WakeupComparator>
        waitQueue;
    std::mutex waitMutex;

    std::condition_variable tickCv;
    std::mutex tickMutex;

    std::vector<std::thread> cpuWorkers;
    std::atomic<uint64_t> cpuCycles{0};
    std::atomic<bool> running{false};

    std::thread dummyGeneratorThread;
    std::atomic<bool> generatingDummies{false};

    std::thread tickThread;
};
