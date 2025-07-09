#pragma once

#include <cstdint>
#include <string>

enum class SchedulerType {
    FCFS,
    RR,
};

class Config {
public:
    // Meyer's singleton stuff
    static Config& getInstance();
    Config(const Config&) = delete;
    Config(Config&&) = delete;
    Config& operator=(const Config&) = delete;
    Config& operator=(Config&&) = delete;

    bool loadFromFile();
    [[nodiscard]] int getNumCPUs() const;
    [[nodiscard]] SchedulerType getSchedulerType() const;
    [[nodiscard]] uint64_t getQuantumCycles() const;
    [[nodiscard]] uint64_t getBatchProcessFreq() const;
    [[nodiscard]] uint64_t getMinInstructions() const;
    [[nodiscard]] uint64_t getMaxInstructions() const;
    [[nodiscard]] uint64_t getDelaysPerExec() const;
    void print() const;
    [[nodiscard]] uint64_t getMaxOverallMem() const;
    [[nodiscard]] uint64_t getMemPerFrame() const;
    [[nodiscard]] uint64_t getMinMemPerProc() const;
    [[nodiscard]] uint64_t getMaxMemPerProc() const;
    [[nodiscard]] uint64_t getMemPerProc() const;

private:
    // Private constructor to prevent instantiation
    Config() = default;

    // Default values from the specs
    int delayPerExec = 1;  // Default value if not set in config
    uint8_t numCPUs = 4;
    SchedulerType scheduler = SchedulerType::RR;
    uint32_t quantumCycles = 5;
    uint32_t batchProcessFreq = 1;
    uint32_t minInstructions = 1000;
    uint32_t maxInstructions = 2000;
    uint32_t delaysPerExec = 0;

    // New memory-related config values
    uint32_t maxOverallMem = 1024;
    uint32_t memPerFrame = 64;
    uint32_t minMemPerProc = 64;
    uint32_t maxMemPerProc = 1024;
    uint32_t memPerProc = 64;

    bool delayEnabled = false;
};
