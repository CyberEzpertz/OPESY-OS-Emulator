#pragma once

#include <atomic>
#include <barrier>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "Config.h"
#include "Process.h"

// For the min-heap waiting queue
struct WakeupComparator {
    bool operator()(const std::shared_ptr<Process>& a, const std::shared_ptr<Process>& b) const {
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
    uint64_t getIdleCPUTicks() const;
    uint64_t getActiveCPUTicks() const;
    std::vector<std::shared_ptr<Process>> getCoreAssignments() const;
    void initialize();
    void scheduleProcess(const std::shared_ptr<Process>& process);
    void sleepProcess(const std::shared_ptr<Process>& process);
    uint64_t getTotalCPUTicks() const;
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
    void incrementCpuTicks();
    void dummyGeneratorLoop();
    void executeFCFS(const std::shared_ptr<Process>& proc, uint64_t& lastTickSeen);
    void executeRR(const std::shared_ptr<Process>& proc, uint64_t& lastTickSeen);

    // Memory management methods
    bool tryAllocateMemory(std::shared_ptr<Process>& proc);
    void deallocateProcessMemory(const std::shared_ptr<Process>& proc) const;
    void resetCore(std::shared_ptr<Process>& proc, int coreId);

    int numCpuCores;
    std::atomic<int> availableCores;
    std::vector<std::shared_ptr<Process>> coreAssignments;

    std::deque<std::shared_ptr<Process>> readyQueue;
    std::mutex readyMutex;

    std::priority_queue<std::shared_ptr<Process>, std::vector<std::shared_ptr<Process>>, WakeupComparator> waitQueue;
    std::mutex waitMutex;

    std::condition_variable tickCv;
    std::mutex tickMutex;
    std::atomic<int> coresFinishedThisTick = 0;
    std::unique_ptr<std::barrier<std::function<void()>>> tickBarrier;

    std::vector<std::thread> cpuWorkers;
    std::atomic<uint64_t> totalCPUTicks{0};
    std::atomic<uint64_t> activeCpuTicks = 0;
    std::atomic<uint64_t> idleCpuTicks = 0;
    std::atomic<bool> running{false};

    std::thread dummyGeneratorThread;
    std::atomic<bool> generatingDummies{false};

    std::thread tickThread;
};