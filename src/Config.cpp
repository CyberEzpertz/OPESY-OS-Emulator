#include "Config.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <cmath>
#include <print>

std::string stripQuotes(const std::string& input) {
    if (input.size() >= 2 && input.front() == '"' && input.back() == '"') {
        return input.substr(1, input.size() - 2);
    }
    return input;
}

Config& Config::getInstance() {
    static Config instance;

    return instance;
}

// Helper function to clamp memory values to the nearest valid power of 2
uint64_t clampToValidMemoryValue(uint64_t value, const std::string& paramName) {
    uint64_t originalValue = value;
    value = std::clamp(value, uint64_t{64}, uint64_t{65536});

    if (value == 0) return 64;

    uint64_t result = 1;
    while (result <= value / 2) {
        result <<= 1;
    }

    if (originalValue != result) {
        std::println("Warning: {} value {} is not a valid power of 2 in range [16, 65536]. Clamping to {}.",
                 paramName, originalValue, result);
    }

    return result;
}

bool Config::loadFromFile() {
    const std::string& filePath = "../config.txt";
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config file: " << filePath << "\n";
        return false;
    }

    std::unordered_map<std::string, std::function<void(std::ifstream&)>> pairs =
        {{"num-cpu",
          [this](std::ifstream& f) {
              int value;
              f >> value;
              numCPUs = std::clamp(value, 1, 128);
          }},
         {"quantum-cycles",
          [this](std::ifstream& f) {
              int64_t value;
              f >> value;

              value = value > 0 ? value - 1 : 0;
              quantumCycles = static_cast<uint32_t>(
                  std::clamp(value, int64_t{0},
                             int64_t{std::numeric_limits<uint32_t>::max()}));
          }},
         {"batch-process-freq",
          [this](std::ifstream& f) {
              int64_t value;
              f >> value;

              value = value > 0 ? value - 1 : 0;
              batchProcessFreq = static_cast<uint32_t>(
                  std::clamp(value, int64_t{0},
                             int64_t{std::numeric_limits<uint32_t>::max()}));
          }},
         {"min-ins",
          [this](std::ifstream& f) {
              int64_t value;
              f >> value;

              value = value > 0 ? value - 1 : 0;
              minInstructions = static_cast<uint32_t>(
                  std::clamp(value, int64_t{0},
                             int64_t{std::numeric_limits<uint32_t>::max()}));
          }},
         {"max-ins",
          [this](std::ifstream& f) {
              int64_t value;
              f >> value;

              value = value > 0 ? value - 1 : 0;
              maxInstructions = static_cast<uint32_t>(
                  std::clamp(value, int64_t{0},
                             int64_t{std::numeric_limits<uint32_t>::max()}));
          }},
         {"delays-per-exec",
          [this](std::ifstream& f) {
              int64_t value;
              f >> value;

              delayEnabled = value > 0;
              value = value > 0 ? value - 1 : 0;

              delaysPerExec = static_cast<uint32_t>(
                  std::clamp(value, int64_t{0},
                             int64_t{std::numeric_limits<uint32_t>::max()}));
          }},
         {"scheduler", [this](std::ifstream& f) {
              std::string sched;
              f >> sched;
              sched = stripQuotes(sched);
              std::ranges::transform(sched, sched.begin(), ::tolower);

              if (sched == "fcfs")
                  scheduler = SchedulerType::FCFS;
              else if (sched == "rr")
                  scheduler = SchedulerType::RR;
              // Else, stick to default
         }},
        {"max-overall-mem",
          [this](std::ifstream& f) {
              uint64_t value;
              f >> value;
              maxOverallMem = clampToValidMemoryValue(value, "max-overall-mem");
          }},
         {"mem-per-frame",
          [this](std::ifstream& f) {
              uint64_t value;
              f >> value;
              memPerFrame = clampToValidMemoryValue(value, "mem-per-frame");
          }},
         {"min-mem-per-proc",
          [this](std::ifstream& f) {
              uint64_t value;
              f >> value;
              minMemPerProc = clampToValidMemoryValue(value, "min-mem-per-proc");
          }},
         {"max-mem-per-proc",
          [this](std::ifstream& f) {
              uint64_t value;
              f >> value;
              maxMemPerProc = clampToValidMemoryValue(value, "max-mem-per-proc");
          }},
         {"mem-per-proc",
          [this](std::ifstream& f) {
              uint64_t value;
              f >> value;
              memPerProc = clampToValidMemoryValue(value, "mem-per-proc");
     }}};

    std::string key;
    while (file >> key) {
        if (auto it = pairs.find(key); it != pairs.end()) {
            it->second(file);
        } else {
            std::cerr << "Warning: Unknown config key '" << key << "'\n";
            std::string skip;
            file >> skip;
        }
    }
    return true;
}

int Config::getNumCPUs() const {
    return numCPUs;
}

SchedulerType Config::getSchedulerType() const {
    return scheduler;
}

uint64_t Config::getQuantumCycles() const {
    return quantumCycles + 1;
}

uint64_t Config::getBatchProcessFreq() const {
    return batchProcessFreq + 1;
}

uint64_t Config::getMinInstructions() const {
    return minInstructions + 1;
}

uint64_t Config::getMaxInstructions() const {
    return maxInstructions + 1;
}

uint64_t Config::getDelaysPerExec() const {
    return delaysPerExec + (delayEnabled ? 1 : 0);
}


uint64_t Config::getMaxOverallMem() const {
    return maxOverallMem;
}
uint64_t Config::getMemPerFrame() const {
    return memPerFrame;
}
uint64_t Config::getMinMemPerProc() const {
    return minMemPerProc;
}
uint64_t Config::getMaxMemPerProc() const {
    return maxMemPerProc;
}
uint64_t Config::getMemPerProc() const {
    return memPerProc;
}

void Config::print() const {
    std::cout << "=== Loaded Configuration ===\n";
    std::cout << "Number of CPUs       : " << getNumCPUs() << '\n';
    std::cout << "Scheduler            : "
              << (scheduler == SchedulerType::FCFS ? "FCFS" : "RR") << '\n';
    std::cout << "Quantum Cycles       : " << getQuantumCycles() << '\n';
    std::cout << "Batch Process Freq   : " << getBatchProcessFreq() << '\n';
    std::cout << "Min Instructions     : " << getMinInstructions() << '\n';
    std::cout << "Max Instructions     : " << getMaxInstructions() << '\n';
    std::cout << "Delays per Execution : " << getDelaysPerExec() << '\n';
    std::cout << "Max Overall Mem      : " << getMaxOverallMem() << '\n';
    std::cout << "Mem per Frame        : " << getMemPerFrame() << '\n';
    std::cout << "Min Mem per Proc     : " << getMinMemPerProc() << '\n';
    std::cout << "Max Mem per Proc     : " << getMaxMemPerProc() << '\n';
    std::cout << "Fixed Mem per Proc   : " << getMemPerProc() << '\n';
    std::cout << "=============================\n";
}