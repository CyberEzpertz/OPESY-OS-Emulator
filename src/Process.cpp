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
#include <set>
#include <shared_mutex>
#include <sstream>

#include "Config.h"
#include "PagingAllocator.h"

// If INSTRUCTION_SIZE is 0, we assume it doesn't count toward paging
constexpr int INSTRUCTION_SIZE = 0;

constexpr int MAX_VARIABLES = 32;
constexpr int VARIABLE_SIZE = 2;

// Common constructor with all parameters
Process::Process(const int id, const std::string& name, const uint64_t requiredMemory)
    : processID(id),
      processName(name),
      currentLine(0),
      requiredMemory(requiredMemory),
      currentInstructionIndex(0),
      status(READY),
      currentCore(-1),
      variables({}),
      wakeupTick(0),
      maxHeapMemory(0) {
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
        // [IMPORTANT] This commented code is if instructions are used in paging, uncomment if needed
        // const int instructionPage = (currentLine * INSTRUCTION_SIZE) / Config::getInstance().getMemPerFrame();
        //
        // if (!pageTable[instructionPage].isValid) {
        //     PagingAllocator::getInstance().handlePageFault(this->processID, instructionPage);
        // }

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

    const int pageSize = static_cast<int>(Config::getInstance().getMemPerFrame());

    const size_t numPages = requiredMemory / pageSize;
    pageTable.resize(numPages);

    // Calculate where the heap begins

    const int symbolTableStartByte = totalLines * INSTRUCTION_SIZE;
    const int symbolTableEndByte = symbolTableStartByte + MAX_VARIABLES * 2;

    const int symbolStartPage = symbolTableStartByte / pageSize;
    const int symbolEndPage = (symbolTableEndByte - 1) / pageSize;  // inclusive

    // Populate symbolTablePages set
    for (int page = symbolStartPage; page <= symbolEndPage; ++page) {
        symbolTablePages.insert(page);
    }

    heapStartPage = symbolTableEndByte / pageSize;
    heapStartOffset = symbolTableEndByte % pageSize;

    const int heapBytes = numPages * pageSize - (heapStartPage * pageSize + heapStartOffset);
    heapMemory.resize(heapBytes);
}

bool Process::setVariable(const std::string& name, const uint16_t value) {
    std::lock_guard lock(scopeMutex);

    for (const int page : symbolTablePages) {
        if (!pageTable[page].isValid) {
            PagingAllocator::getInstance().handlePageFault(processID, page);
        }
    }

    if (variables.contains(name)) {
        variables[name] = value;
        return true;
    }

    return false;
}

uint16_t Process::getVariable(const std::string& name) {
    std::lock_guard lock(scopeMutex);

    for (const int page : symbolTablePages) {
        if (!pageTable[page].isValid) {
            PagingAllocator::getInstance().handlePageFault(processID, page);
        }
    }

    if (variables.contains(name)) {
        return variables[name];
    }

    if (variables.size() < MAX_VARIABLES) {
        variables[name] = 0;
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

bool Process::declareVariable(const std::string& name, uint16_t value) {
    std::lock_guard lock(scopeMutex);

    for (const int page : symbolTablePages) {
        if (!pageTable[page].isValid) {
            PagingAllocator::getInstance().handlePageFault(processID, page);
        }
    }

    // If we're pass max variables, just ignore according to project specs
    if (variables.size() >= MAX_VARIABLES) {
        return true;
    }

    // Don't allow double declarations of the same variable
    if (variables.contains(name)) {
        return false;
    }

    variables[name] = value;
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

PageEntry Process::getPageEntry(const int pageNumber) const {
    return pageTable[pageNumber];
}

void Process::swapPageOut(const int pageNumber) {
    pageTable[pageNumber].isValid = false;
    pageTable[pageNumber].inBackingStore = true;
    pageTable[pageNumber].frameNumber = -1;
}

void Process::swapPageIn(const int pageNumber, const int frameNumber) {
    pageTable[pageNumber].isValid = true;
    pageTable[pageNumber].inBackingStore = false;
    pageTable[pageNumber].frameNumber = frameNumber;
}

void Process::shutdown(int invalidAddress) {
    didShutdown = true;
    shutdownDetails =
        std::format("Process {} shut down due to memory access violation error that occurred at {}. {:x} invalid.",
                    processName, getTimestamp(), invalidAddress);
    status = DONE;
}

// Converts the virtual memory address into the heap index
int Process::convertAddressToHeapIdx(const int address) const {
    const auto pageSize = Config::getInstance().getMemPerFrame();
    const int heapAddressStart = heapStartPage * pageSize + heapStartOffset;

    // Align it so that it doesn't break
    if (address % 2 == 1) {
        return (address - 1) - heapAddressStart;
    }

    return address - heapAddressStart;
}

// Checks if the given address is inside the heap
bool Process::isValidHeapAddress(const int address) const {
    const auto pageSize = Config::getInstance().getMemPerFrame();
    const int heapAddressStart = heapStartPage * pageSize + heapStartOffset;

    return address > heapAddressStart && address < requiredMemory;
}

// Writes to given address if possible, shuts down if not
void Process::writeToHeap(const int address, const uint16_t value) {
    const auto pageSize = Config::getInstance().getMemPerFrame();
    const int totalOffset = heapStartOffset + address;
    const int pageNumber = heapStartPage + (totalOffset / pageSize);

    // Invalid memory access criteria:
    // 1. Address is outside of heap bounds
    if (isValidHeapAddress(address)) {
        shutdown(address);
        return;
    }

    if (!pageTable[pageNumber].isValid) {
        PagingAllocator::getInstance().handlePageFault(processID, pageNumber);
    }

    // Convert raw virtual memory address into index
    const auto heapIndex = convertAddressToHeapIdx(address);

    heapMemory[heapIndex] = value & 0xFF;             // lower byte
    heapMemory[heapIndex + 1] = (value >> 8) & 0xFF;  // upper byte
}

// Reads the given address if possible, shuts down if not
uint16_t Process::readFromHeap(const int address) {
    const auto pageSize = Config::getInstance().getMemPerFrame();
    const int totalOffset = heapStartOffset + address;
    const int pageNumber = heapStartPage + (totalOffset / pageSize);

    // Invalid memory access criteria:
    // 1. Address is outside of heap bounds
    // 2. Address is inside, but it's an odd number
    if (isValidHeapAddress(address)) {
        shutdown(address);
        return 0;
    }

    if (!pageTable[pageNumber].isValid) {
        PagingAllocator::getInstance().handlePageFault(processID, pageNumber);
    }

    const auto heapIndex = convertAddressToHeapIdx(address);

    // Combine the two halves here
    return heapMemory[heapIndex] | (heapMemory[heapIndex + 1] << 8);
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

std::uint64_t Process::getMemoryUsage() const {
    std::lock_guard lock(scopeMutex);
    uint64_t memoryUsage = 0;
    const auto pageSize = Config::getInstance().getMemPerFrame();

    for (const auto page : pageTable) {
        if (page.isValid)
            memoryUsage += pageSize;
    }

    return memoryUsage;
}