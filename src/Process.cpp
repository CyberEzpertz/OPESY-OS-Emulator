//
// Created by paren on 22/05/2025.
//

#include "Process.h"

#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <set>
#include <shared_mutex>
#include <sstream>

#include "Config.h"
#include "PagingAllocator.h"

// If INSTRUCTION_SIZE is 0, we assume it doesn't count toward paging
constexpr int INSTRUCTION_SIZE = 2;
constexpr int MAX_VARIABLES = 32;
constexpr int VARIABLE_SIZE = 2;

// Common constructor with all parameters
Process::Process(const int id, const std::string& name, const uint64_t requiredMemory)
    : processID(id),
      processName(name),
      currentLine(0),
      totalLines(0),
      requiredMemory(requiredMemory),
      currentInstructionIndex(0),
      status(READY),
      currentCore(-1),
      wakeupTick(0),
      variableAddresses({}) {
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

void Process::safePageFault(const int page) const {
    PagingAllocator& allocator = PagingAllocator::getInstance();
    if (!pageTable[page].isValid || !allocator.pinFrame(pageTable[page].frameNumber, processID, page)) {
        allocator.handlePageFault(this->processID, page);
    }
}

/**
 * @brief Increments the current line number, up to the total number of lines.
 */
void Process::incrementLine() {
    std::lock_guard lock(instructionsMutex);
    if (currentLine < totalLines) {
        const int instrAddress = (currentInstructionIndex * INSTRUCTION_SIZE);
        const auto [page, offset] = splitAddress(instrAddress);
        safePageFault(page);

        {
            // std::lock_guard pageLock(pageTableMutex);
            auto frameNumber = pageTable[page].frameNumber;

            const auto instr = PagingAllocator::getInstance().readFromFrame(frameNumber, offset);

            if (!std::holds_alternative<std::shared_ptr<Instruction>>(instr)) {
                throw std::runtime_error(std::format("Frame {} Offset {} is not an instruction.", frameNumber, offset));
            }

            const auto parsedInstr = std::get<std::shared_ptr<Instruction>>(instr);
            parsedInstr->execute();

            currentLine++;

            if (parsedInstr->isComplete())
                currentInstructionIndex++;
        }
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
void Process::setInstructions(const std::vector<std::shared_ptr<Instruction>>& instructions, const bool addToMemory) {
    std::lock_guard lock(instructionsMutex);

    // Set instruction-related props
    this->instructions = instructions;
    this->totalLines = 0;

    const int instructionBytes = instructions.size() * INSTRUCTION_SIZE;
    for (const auto& instr : instructions) {
        this->totalLines += instr->getLineCount();
    }

    // In the case that instructions are not counted in initial required memory
    // We can set this flag to be true
    if (addToMemory) {
        requiredMemory += instructionBytes;
    }

    // Initialize pageTable
    const int pageSize = static_cast<int>(Config::getInstance().getMemPerFrame());
    const size_t numPages = std::ceil(static_cast<double>(requiredMemory) / pageSize);
    pageTable.resize(numPages);

    segmentBoundaries[TEXT] = instructionBytes;
    segmentBoundaries[DATA] = segmentBoundaries[TEXT] + MAX_VARIABLES * VARIABLE_SIZE;
    segmentBoundaries[HEAP] = requiredMemory;
}

bool Process::setVariable(const std::string& name, const uint16_t value) {
    std::lock_guard lock(variableMutex);

    // Variable must already be declared
    if (!variableAddresses.contains(name)) {
        return false;
    }

    const auto address = variableAddresses[name];
    const auto [page, offset] = splitAddress(address);

    safePageFault(page);

    const auto frame = pageTable[page].frameNumber;
    PagingAllocator::getInstance().writeToFrame(frame, offset, value);

    return true;
}

uint16_t Process::getVariable(const std::string& name) {
    std::lock_guard lock(variableMutex);

    // CASE 1: Variable has been declared already
    if (variableAddresses.contains(name)) {
        const auto [page, offset] = splitAddress(variableAddresses[name]);

        safePageFault(page);

        const auto frameNumber = pageTable[page].frameNumber;
        const auto data = PagingAllocator::getInstance().readFromFrame(frameNumber, offset);

        if (std::holds_alternative<uint16_t>(data))
            return std::get<uint16_t>(data);

        throw std::runtime_error(std::format("Variable '{}' at address 0x{:04X} is not a uint16_t", name,
                                             static_cast<int>(variableAddresses[name])));
    }

    // CASE 2: Variable has not been declared yet
    const auto symbolTableStart = segmentBoundaries[TEXT];

    // CASE 2.1: Symbol Table limit reached
    if (variableOrder.size() >= MAX_VARIABLES) {
        return 0;
    }

    // CASE 2.2: Variable can be declared
    if (symbolTableStart + variableOrder.size() * VARIABLE_SIZE > requiredMemory) {
        throw std::runtime_error("Tried to declare new variable while reading but memory exceeded");
    }

    const uint16_t nextAddress = symbolTableStart + variableOrder.size() * VARIABLE_SIZE;
    variableAddresses[name] = nextAddress;
    variableOrder.push_back(name);

    // Return 0 since it's an uninitialized variable
    return 0;
}

bool Process::getIsFinished() const {
    return currentLine >= totalLines || status == DONE;
}

uint64_t Process::getWakeupTick() const {
    return wakeupTick;
}
void Process::setWakeupTick(const uint64_t value) {
    this->wakeupTick = value;
}

bool Process::declareVariable(const std::string& name, uint16_t value) {
    std::lock_guard lock(variableMutex);

    // Don't allow double declarations
    if (variableAddresses.contains(name)) {
        return false;
    }

    // If we've reached max variables, ignore as per spec
    if (variableAddresses.size() >= MAX_VARIABLES) {
        return true;
    }

    const auto symbolTableStart = segmentBoundaries[TEXT];
    const uint16_t nextAddress = symbolTableStart + variableOrder.size() * VARIABLE_SIZE;

    // Ensure we don't exceed required memory
    if (nextAddress >= requiredMemory) {
        throw std::runtime_error("Memory exceeded during variable declaration");
    }

    const auto [page, offset] = splitAddress(nextAddress);

    // Ensure page is loaded
    safePageFault(page);

    // Write initial value
    const auto frame = pageTable[page].frameNumber;
    PagingAllocator::getInstance().writeToFrame(frame, offset, value);

    // Track variable
    variableAddresses[name] = nextAddress;
    variableOrder.push_back(name);

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
    std::lock_guard lock(pageTableMutex);
    pageTable[pageNumber].isValid = false;
    pageTable[pageNumber].inBackingStore = true;
    pageTable[pageNumber].frameNumber = -1;
}

void Process::swapPageIn(const int pageNumber, const int frameNumber) {
    std::lock_guard lock(pageTableMutex);
    pageTable[pageNumber].isValid = true;
    pageTable[pageNumber].inBackingStore = false;
    pageTable[pageNumber].frameNumber = frameNumber;
}

void Process::shutdown(int invalidAddress) {
    didShutdown = true;

    // Extract only the HH:MM:SS part from timestamp
    std::string timeOnly = getTimestamp();
    auto commaPos = timeOnly.find(',');
    if (commaPos != std::string::npos) {
        timeOnly = timeOnly.substr(commaPos + 2);  // Skip ", "
    }

    shutdownDetails =
        std::format("Process {} shut down due to memory access violation error that occurred at {}. 0x{:X} invalid.",
                    processName, timeOnly, invalidAddress);

    status = DONE;
}

std::pair<int, int> Process::splitAddress(const int address) {
    const int pageSize = Config::getInstance().getMemPerFrame();
    const int page = address / pageSize;
    const int offset = address % pageSize;

    return {page, offset};
}

// Checks if the given address is inside the heap
bool Process::isValidHeapAddress(const int address) const {
    return address >= segmentBoundaries.at(DATA) && address < segmentBoundaries.at(HEAP);
}

// Writes to given address if possible, shuts down if not
void Process::writeToHeap(const int address, const uint16_t value) {
    std::lock_guard lock(heapMutex);

    // Invalid memory access criteria:
    // 1. Address is outside of heap bounds
    if (!isValidHeapAddress(address)) {
        shutdown(address);
        return;
    }

    auto [page, offset] = splitAddress(address);

    // Align memory to 16-bit boundary
    if (offset % 2 == 1) {
        offset -= 1;
    }

    // Ensure the page is loaded
    safePageFault(page);

    const auto frameNumber = pageTable[page].frameNumber;

    PagingAllocator::getInstance().writeToFrame(frameNumber, offset, value);
}

PageData Process::getPageData(const int pageNumber) const {
    const auto pageSize = Config::getInstance().getMemPerFrame();
    const int start = pageNumber * pageSize;
    const int end = start + pageSize;

    std::vector<std::optional<StoredData>> data;
    // Iterate through the memory
    for (int i = start; i < end; i += 2) {
        if (i < segmentBoundaries.at(TEXT)) {
            auto instruction = instructions[i / INSTRUCTION_SIZE];
            data.push_back(instruction);
            data.push_back(std::nullopt);
        } else {
            // Will be 0 because no variables/memory has been written to yet
            data.push_back(static_cast<uint16_t>(0));
            data.push_back(static_cast<uint16_t>(0));
        }
    }

    return data;
}

// Reads the given address if possible, shuts down if not
uint16_t Process::readFromHeap(const int address) {
    std::lock_guard lock(heapMutex);
    // Invalid memory access criteria:
    // 1. Address is outside of heap bounds
    if (!isValidHeapAddress(address)) {
        shutdown(address);
        return 0;
    }

    auto [page, offset] = splitAddress(address);

    // Align memory
    if (offset % 2 == 1) {
        offset -= 1;
    }

    // Ensure page is loaded
    safePageFault(page);

    const auto frameNumber = pageTable[page].frameNumber;

    const auto data = PagingAllocator::getInstance().readFromFrame(frameNumber, offset);

    if (std::holds_alternative<uint16_t>(data))
        return std::get<uint16_t>(data);

    throw std::runtime_error(std::format("Data at address 0x{:04X} is not a uint16_t", address));
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
std::string Process::generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t timeT = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&timeT);

    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%m/%d/%Y, %I:%M:%S %p");
    return oss.str();
}

std::uint64_t Process::getMemoryUsage() const {
    // std::lock_guard lock(variableMutex);
    uint64_t memoryUsage = 0;
    const auto pageSize = Config::getInstance().getMemPerFrame();

    for (const auto page : pageTable) {
        if (page.isValid)
            memoryUsage += pageSize;
    }

    return memoryUsage;
}

void Process::precomputeInstructionPages() {
    const auto pageSize = Config::getInstance().getMemPerFrame();
    std::vector<PageData> pages;

    PageData currentPage(pageSize, std::nullopt);
    size_t offset = 0;

    for (const auto& instr : instructions) {
        const size_t size = instr->getLineCount() * INSTRUCTION_SIZE;

        for (size_t i = 0; i < size; ++i) {
            if (offset == pageSize) {
                pages.push_back(std::move(currentPage));
                currentPage = PageData(pageSize, std::nullopt);
                offset = 0;
            }

            if (i == 0) {
                currentPage[offset++] = StoredData{instr};  // first byte points to instruction
            } else {
                currentPage[offset++] = std::nullopt;  // claimed, but not instruction start
            }
        }
    }

    if (offset > 0) {
        pages.push_back(std::move(currentPage));
    }

    this->precomputedPages = std::move(pages);
}