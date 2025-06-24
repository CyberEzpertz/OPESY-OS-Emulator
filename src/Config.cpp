#include "Config.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <limits>

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
        {{"num-cpu", [this](std::ifstream& f) { f >> numCPUs; }},
         {"quantum-cycles", [this](std::ifstream& f) {
             uint64_t value;
             f >> value;
             if (value > 0) {
                 value = value - 1;
                 quantumCycles = std::min(value, static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()));
             } else {
                 quantumCycles = 0;
             }
         }},
         {"batch-process-freq", [this](std::ifstream& f) {
             uint64_t value;
             f >> value;
             if (value > 0) {
                 value = value - 1;
                 batchProcessFreq = std::min(value, static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()));
             } else {
                 batchProcessFreq = 0;
             }
         }},
         {"min-ins", [this](std::ifstream& f) {
             uint64_t value;
             f >> value;
             if (value > 0) {
                 value = value - 1;
                 minInstructions = std::min(value, static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()));
             } else {
                 minInstructions = 0;
             }
         }},
         {"max-ins", [this](std::ifstream& f) {
             uint64_t value;
             f >> value;
             if (value > 0) {
                 value = value - 1;
                 maxInstructions = std::min(value, static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()));
             } else {
                 maxInstructions = 0;
             }
         }},
         {"delays-per-exec", [this](std::ifstream& f) {
             uint64_t value;
             f >> value;
             if (value > 0) {
                 value = value - 1;
                 delaysPerExec = std::min(value, static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()));
             } else {
                 delaysPerExec = 0;
             }
         }},
         {"scheduler", [this](std::ifstream& f) {
              std::string sched;
              f >> sched;
              sched = stripQuotes(sched);  // ✅ remove the quotes
              std::ranges::transform(sched, sched.begin(), ::tolower);

              if (sched == "fcfs")
                  scheduler = SchedulerType::FCFS;
              else if (sched == "rr")
                  scheduler = SchedulerType::RR;

              // If none of the above, just sticks to the default value
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
    return numCPUs + 1;
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
    return delaysPerExec + 1;
}
void Config::print() const {
    std::cout << "=== Loaded Configuration ===\n";
    std::cout << "Number of CPUs       : " << numCPUs << '\n';
    std::cout << "Scheduler            : "
              << (scheduler == SchedulerType::FCFS ? "FCFS" : "RR") << '\n';
    std::cout << "Quantum Cycles       : " << quantumCycles << '\n';
    std::cout << "Batch Process Freq   : " << batchProcessFreq << '\n';
    std::cout << "Min Instructions     : " << minInstructions << '\n';
    std::cout << "Max Instructions     : " << maxInstructions << '\n';
    std::cout << "Delays per Execution : " << delaysPerExec << '\n';
    std::cout << "=============================\n";
}