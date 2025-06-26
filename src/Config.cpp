#include "Config.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <unordered_map>

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

uint32_t Config::getQuantumCycles() const {
    return quantumCycles + 1;
}

uint32_t Config::getBatchProcessFreq() const {
    return batchProcessFreq + 1;
}

uint32_t Config::getMinInstructions() const {
    return minInstructions + 1;
}

uint32_t Config::getMaxInstructions() const {
    return maxInstructions + 1;
}

uint32_t Config::getDelaysPerExec() const {
    return delaysPerExec + (delayEnabled ? 1 : 0);
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
    std::cout << "=============================\n";
}