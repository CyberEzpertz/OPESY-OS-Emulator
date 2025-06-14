#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

enum class SchedulerType { FCFS, RR };

class Process;  // Forward declaration

class ProcessScheduler {
public:
    static ProcessScheduler& getInstance();

    void start();
    void initialize(int numCores);  // TODO
    void scheduleProcess(const std::shared_ptr<Process>& process);
    void sortQueue();  // FCFS only
    uint64_t getCurrentCycle() const;

private:
    ProcessScheduler();
    ~ProcessScheduler();

    ProcessScheduler(const ProcessScheduler&) = delete;
    ProcessScheduler& operator=(const ProcessScheduler&) = delete;

    void tickLoop();
    void workerLoop(int coreId);
    void incrementCpuCycles();

    int numCpuCores = 4;
    SchedulerType schedulerType = SchedulerType::FCFS;
    uint32_t quantumCycles = 1;

    std::deque<std::shared_ptr<Process>> processQueue;
    std::mutex queueMutex;

    std::condition_variable tickCv;
    std::mutex tickMutex;

    std::vector<std::thread> cpuWorkers;
    std::atomic<uint64_t> cpuCycles{0};
    std::atomic<bool> running{false};

    std::thread tickThread;
};
