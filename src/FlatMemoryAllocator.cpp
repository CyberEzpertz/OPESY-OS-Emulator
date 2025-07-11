#include "FlatMemoryAllocator.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_set>

#include "Config.h"
#include "Process.h"

FlatMemoryAllocator& FlatMemoryAllocator::getInstance() {
    static FlatMemoryAllocator instance;

    return instance;
}

void* FlatMemoryAllocator::allocate(const size_t size, std::shared_ptr<Process> process) {
    std::lock_guard lock(memoryMutex);

    // Loop through memory from index 0 to (maximumSize - size)
    for (size_t i = 0; i <= maximumSize - size; ++i) {
        if (canAllocateAt(i, size)) {
            allocateAt(i, size, process->getName());
            allocatedSize += size;

            void* baseAddress = &memoryView[i];
            process->setBaseAddress(baseAddress);

            return baseAddress;  // Return pointer to the allocated block
        }
    }

    // No space available
    return nullptr;
}

void FlatMemoryAllocator::deallocate(void* ptr, const std::shared_ptr<Process> process) {
    const auto memIdx = static_cast<char*>(ptr) - &memoryView[0];

    // Double check if it's inside the memoryMap itself
    if (!memoryMap[memIdx].empty()) {
        process->setBaseAddress(nullptr);
        deallocateAt(memIdx, process->getRequiredMemory());
    }
}

size_t FlatMemoryAllocator::getProcessMemoryUsage(const std::string& processName) const {
    return std::count(memoryMap.begin(), memoryMap.end(), processName);
}
size_t FlatMemoryAllocator::getTotalMemoryUsage() const {
    return allocatedSize;
}
void FlatMemoryAllocator::visualizeMemory(int quantumCycle) {
    std::lock_guard lock(memoryMutex);

    // Build filename
    std::string filename = std::format("logs/memory_stamp_{}.txt", quantumCycle);
    std::filesystem::create_directories("logs");  // 🔥 Fix: Create logs/ folder if missing
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not write memory snapshot to " << filename << "\n";
        return;
    }

    // Generate timestamp
    auto now = std::chrono::system_clock::now();
    std::time_t timeT = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&timeT);
    std::ostringstream timestamp;
    timestamp << std::put_time(&local_tm, "%m/%d/%Y %I:%M:%S %p");

    // Count processes in memory
    std::unordered_set<std::string> processNames;
    for (const auto& name : memoryMap) {
        if (!name.empty()) {
            processNames.insert(name);
        }
    }

    size_t externalFrag = maximumSize - allocatedSize;

    // Header Info
    outFile << "Timestamp: " << timestamp.str() << "\n";
    outFile << "Number of processes in memory: " << processNames.size() << "\n";
    outFile << "Total external fragmentation in KB: " << externalFrag << "\n\n";

    // Memory layout (descending order)
    outFile << "Memory Layout:\n";
    outFile << "---end--- = " << maximumSize << " \n";

    size_t index = maximumSize;
    while (index > 0) {
        size_t blockEnd = index;
        std::string label = memoryMap[index - 1];
        bool isFree = label.empty();

        size_t blockSize = 0;
        while (index > 0) {
            --index;
            if ((memoryMap[index].empty() && !isFree) ||
                (!memoryMap[index].empty() && (memoryMap[index] != label))) {
                ++index;  // step back to the last correct index
                break;
            }
            ++blockSize;
        }

        outFile << blockEnd << " KB\n";
        outFile << (isFree ? "FREE" : label) << "\n";
        outFile << (blockEnd - blockSize) << " \n\n";
    }

    outFile << "---start--- = 0 \n";

    outFile.close();
}

size_t FlatMemoryAllocator::getAllocatedSize() const {
    return allocatedSize;
}
size_t FlatMemoryAllocator::getMaximumSize() const {
    return maximumSize;
}

FlatMemoryAllocator::FlatMemoryAllocator() : allocatedSize(0) {
    // This is assuming config either has a default or has been initialized
    maximumSize = Config::getInstance().getMaxOverallMem();
    memoryView.resize(maximumSize, '.');
    memoryMap.resize(maximumSize, "");
}

bool FlatMemoryAllocator::canAllocateAt(const size_t index, const size_t size) const {
    if (index + size > maximumSize)
        return false;

    for (size_t i = index; i < index + size; ++i) {
        if (memoryView[i] != '.') {
            return false;  // Block is occupied by a process
        }
    }

    return true;
}
void FlatMemoryAllocator::allocateAt(const size_t index, const size_t size, const std::string& processName) {
    // Just in case there exists a bug
    if (index + size > memoryMap.size()) {
        throw std::out_of_range("Attempted to allocate beyond memory bounds.");
    }

    std::ranges::fill(memoryMap.begin() + index, memoryMap.begin() + index + size, processName);
    std::ranges::fill(memoryView.begin() + index, memoryView.begin() + index + size, '+');
    allocatedSize += size;
}

void FlatMemoryAllocator::deallocateAt(const size_t index, const size_t size) {
    std::ranges::fill(memoryMap.begin() + index, memoryMap.begin() + index + size, "");
    std::ranges::fill(memoryView.begin() + index, memoryView.begin() + index + size, '.');
    allocatedSize -= size;
}