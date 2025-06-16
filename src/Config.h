#pragma once

#include <cstdint>
#include <string>

enum class SchedulerType {
    FCFS,
    RR,
};

class Config {
public:
    bool loadFromFile(const std::string& filePath);

    [[nodiscard]] int getNumCPUs() const;
    [[nodiscard]] SchedulerType getSchedulerType() const;
    [[nodiscard]] uint32_t getQuantumCycles() const;
    [[nodiscard]] uint32_t getBatchProcessFreq() const;
    [[nodiscard]] uint32_t getMinInstructions() const;
    [[nodiscard]] uint32_t getMaxInstructions() const;
    [[nodiscard]] uint32_t getDelaysPerExec() const;

private:
    // Default values from the specs
    int numCPUs = 4;
    SchedulerType scheduler = SchedulerType::RR;
    uint32_t quantumCycles = 5;
    uint32_t batchProcessFreq = 1;
    uint32_t minInstructions = 1000;
    uint32_t maxInstructions = 2000;
    uint32_t delaysPerExec = 0;
};
