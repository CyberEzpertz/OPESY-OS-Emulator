//
// Created by paren on 22/05/2025.
//

#include "Process.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <shared_mutex>
#include <sstream>

#include "Config.h"

// Common constructor with all parameters
Process::Process(const int id, const std::string& name, const uint64_t requiredMemory)
    : processID(id),
      processName(name),
      currentLine(0),
      requiredMemory(requiredMemory),
      currentInstructionIndex(0),
      status(READY),
      currentCore(-1),
      variableStack({{}}),
      wakeupTick(0) {
    totalLines = 0;
    for (const auto& instr : instructions) {
        totalLines += instr->getLineCount();
    }

    timestamp = generateTimestamp();
}

// Convenience constructor that uses mem-per-proc from config
Process::Process(const int id, const std::string& name) : Process(id, name, Config::getInstance().getMemPerProc()) {
}

/**
 * @brief Gets the unique identifier of the process.
 * @return Process ID.
 */
int Process::getID() const {
    return processID;
}

/**
 * @brief Gets the name of the process.
 * @return A reference to the process name.
 */
std::string Process::getName() const {
    return processName;
}

/**
 * @brief Retrieves the log entries associated with the process.
 * @return A reference to a vector of log strings.
 */
std::vector<std::string> Process::getLogs() const {
    return logs;
}

/**
 * @brief Retrieves the current line the process is executing.
 * @return Current line number.
 */
int Process::getCurrentLine() const {
    return currentLine;
}

/**
 * @brief Retrieves the total number of lines the process is expected to
 * execute.
 * @return Total line count.
 */
int Process::getTotalLines() const {
    return totalLines;
}

/**
 * @brief Gets the timestamp of when the process was created.
 * @return A reference to the timestamp string.
 */
std::string& Process::getTimestamp() {
    return timestamp;
}

/**
 * @brief Adds a new log entry to the process's log.
 * @param entry The log string to add.
 */
void Process::log(const std::string& entry) {
    logs.push_back(entry);
}

/**
 * @brief Increments the current line number, up to the total number of lines.
 */
void Process::incrementLine() {
    std::lock_guard lock(instructionsMutex);
    if (currentLine < totalLines) {
        instructions[currentInstructionIndex]->execute();
        currentLine++;

        if (instructions[currentInstructionIndex]->isComplete())
            currentInstructionIndex++;
    }

    if (currentLine >= totalLines) {
        this->status = DONE;
        instructions.clear();
    }
}

ProcessStatus Process::getStatus() const {
    return status.load();
}

void Process::setStatus(const ProcessStatus newStatus) {
    this->status = newStatus;
}

void Process::setCurrentCore(const int coreId) {
    currentCore = coreId;
}
int Process::getCurrentCore() const {
    return currentCore.load();
}
void Process::setInstructions(const std::vector<std::shared_ptr<Instruction>>& instructions) {
    std::lock_guard lock(instructionsMutex);
    this->instructions = instructions;
    this->totalLines = 0;

    for (const auto& instr : instructions) {
        this->totalLines += instr->getLineCount();
    }
}
bool Process::setVariable(const std::string& name, const uint16_t value) {
    std::lock_guard lock(scopeMutex);

    for (auto it = variableStack.rbegin(); it != variableStack.rend(); ++it) {
        if (it->contains(name)) {
            (*it)[name] = value;
            return true;
        }
    }
    return false;  // Not found in any scope
}

uint16_t Process::getVariable(const std::string& name) {
    std::lock_guard lock(scopeMutex);

    for (auto it = variableStack.rbegin(); it != variableStack.rend(); ++it) {
        if (it->contains(name)) {
            return it->at(name);
        }
    }

    if (!variableStack.empty()) {
        variableStack.back()[name] = 0;
        return 0;
    }
    return 0;
}

bool Process::getIsFinished() const {
    return currentLine >= totalLines;
}

uint64_t Process::getWakeupTick() const {
    return wakeupTick;
}
void Process::setWakeupTick(const uint64_t value) {
    this->wakeupTick = value;
}

void Process::enterScope() {
    std::lock_guard lock(scopeMutex);
    variableStack.emplace_back();
}

void Process::exitScope() {
    std::lock_guard lock(scopeMutex);

    if (!variableStack.empty()) {
        variableStack.pop_back();
    } else {
        std::cout << ("Error: Tried to exit scope but scope stack is empty.") << std::endl;
    }
}

bool Process::declareVariable(const std::string& name, uint16_t value) {
    std::lock_guard lock(scopeMutex);

    if (variableStack.empty())
        return false;
    auto& currentScope = variableStack.back();
    if (currentScope.contains(name)) {
        return false;  // Already declared in this scope
    }
    currentScope[name] = value;
    return true;
}
uint64_t Process::getRequiredMemory() const {
    return requiredMemory;
}
void Process::setBaseAddress(void* ptr) {
    baseAddress = ptr;
}
void* Process::getBaseAddress() const {
    return baseAddress;
}

Page Process::getPageEntry(const int pageNumber) const {
    return pageTable[pageNumber];
}

void Process::swapPageOut(const int pageNumber) {
    pageTable[pageNumber].isValid = false;
    pageTable[pageNumber].inBackingStore = true;
}

void Process::swapPageIn(const int pageNumber) {
    pageTable[pageNumber].isValid = true;
    pageTable[pageNumber].inBackingStore = false;
}

/**
 * @brief Writes all existing log entries to a file under the ./logs directory.
 *
 * Each log line is assumed to already include its own timestamp and CPU core
 * ID. The log file is named using the process name (e.g., logs/p01.txt).
 */
void Process::writeLogToFile() const {
    try {
        // Ensure the logs directory exists
        std::filesystem::create_directories("logs");

        // Build the full path: logs/<processName>.txt
        std::string filename = "logs/" + processName + ".txt";
        std::ofstream outFile(filename);

        if (!outFile.is_open()) {
            std::cerr << "Error: Could not open log file for process " << processName << "\n";
            return;
        }

        outFile << "Process name: " << processName << "\n";
        outFile << "Logs:\n\n";
        // Directly write each pre-formatted log line
        for (const std::string& log : logs) {
            outFile << log << '\n';
        }

        outFile.close();
    } catch (const std::exception& e) {
        std::cerr << "Exception during log writing: " << e.what() << '\n';
    }
}

/**
 * @brief Generates a formatted timestamp string representing the current local
 * time.
 * @return A string formatted as MM/DD/YYYY, HH:MM:SS AM/PM.
 */
std::string Process::generateTimestamp() const {
    auto now = std::chrono::system_clock::now();
    std::time_t timeT = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&timeT);

    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%m/%d/%Y, %I:%M:%S %p");
    return oss.str();
}
