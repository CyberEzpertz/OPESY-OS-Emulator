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
    [[nodiscard]] uint32_t getQuantumCycles() const;
    [[nodiscard]] uint32_t getBatchProcessFreq() const;
    [[nodiscard]] uint32_t getMinInstructions() const;
    [[nodiscard]] uint32_t getMaxInstructions() const;
    [[nodiscard]] uint32_t getDelaysPerExec() const;
    void print() const;

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

    bool delayEnabled = false;
};
