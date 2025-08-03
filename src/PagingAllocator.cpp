//
// Created by Jan on 26/07/2025.
//

#include "PagingAllocator.h"

#include <fstream>
#include <iosfwd>
#include <optional>
#include <print>
#include <variant>
#include <vector>

#include "Config.h"
#include "ConsoleManager.h"
#include "InstructionFactory.h"
#include "Process.h"

static constexpr auto BACKING_STORE_FILE = "csopesy-backing-store.txt";

PagingAllocator& PagingAllocator::getInstance() {
    static auto* instance = new PagingAllocator();
    return *instance;
}

void PagingAllocator::handlePageFault(const int pid, const int pageNumber) {
    std::lock_guard lock(pagingMutex);

    const auto process = ConsoleManager::getInstance().getProcessByPID(pid);

    const PageEntry entry = process->getPageEntry(pageNumber);

    std::vector<std::optional<StoredData>> pageData;

    // If the entry was swapped out, this will trigger
    if (entry.inBackingStore) {
        pageData = swapIn(pid, pageNumber);
    } else {
        pageData = process->getPageData(pageNumber);
    }

    int frameIndex = allocateFrame(pid, pageNumber, pageData);

    // If the allocation was unsuccessful, evict a frame then allocate it again
    if (frameIndex == -1) {
        // Will free up a frame and put it in the free frame list
        evictVictimFrame();

        // This should now succeed since we evicted a frame
        frameIndex = allocateFrame(pid, pageNumber, pageData);

        if (frameIndex == -1) {
            throw std::runtime_error("Frame eviction succeeded, but no frame was available for reallocation — possible "
                                     "free list corruption.");
        }
    }

    process->swapPageIn(pageNumber, frameIndex);
    this->numPagedIn += 1;
}

void PagingAllocator::deallocate(const int pid) {
    std::lock_guard lock(pagingMutex);

    // 1. Free all physical frames used by the process
    for (size_t i = 0; i < frameTable.size(); ++i) {
        if (frameTable[i].pid == pid) {
            freeFrame(static_cast<int>(i));
        }
    }

    // 2. Remove pages from backing store belonging to this process
    std::ifstream inFile(BACKING_STORE_FILE);
    std::ofstream outFile("temp.txt");

    std::string line;
    bool skipLines = false;
    while (std::getline(inFile, line)) {
        std::istringstream iss(line);
        int filePid, pageNumber;

        // Check for a line indicating the start of a swapped page
        if (iss >> filePid >> pageNumber) {
            if (filePid == pid) {
                skipLines = true;
                continue;  // Don't write this line
            }

            // If we're on a new pid now, we're good
            outFile << line << '\n';
            skipLines = false;
            continue;
        }

        // Skip data lines associated with a pid being deallocated
        if (skipLines) {
            continue;
        }

        outFile << line << '\n';
    }

    inFile.close();
    outFile.close();
    std::remove(BACKING_STORE_FILE);
    std::rename("temp.txt", BACKING_STORE_FILE);
}

void PagingAllocator::visualizeMemory() {
    std::println("\n=== Memory Frame Table ===");
    std::println("{:>6} | {:>10} | {:>10}", "Frame", "Process ID", "Page #");
    std::println("--------+------------+------------");

    for (size_t i = 0; i < frameTable.size(); ++i) {
        const auto& [pid, pageNumber, _] = frameTable[i];
        if (pid == -1) {
            std::println("{:>6} | {:>10} | {:>10}", i, "-", "-");
        } else {
            std::println("{:>6} | {:>10} | {:>10}", i, pid, pageNumber);
        }
    }

    std::println("================================\n");
}

int PagingAllocator::getUsedMemory() const {
    const int usedMemory = allocatedFrames * Config::getInstance().getMemPerFrame();
    return usedMemory;
}

int PagingAllocator::getNumPagedIn() const {
    return numPagedIn;
}

int PagingAllocator::getNumPagedOut() const {
    return numPagedOut;
}

int PagingAllocator::getFreeMemory() const {
    return Config::getInstance().getMaxOverallMem() - getUsedMemory();
}

std::optional<uint16_t> PagingAllocator::readUint16FromFrame(const int frameNumber, const int offset) const {
    const auto& data = frameTable[frameNumber].data;

    if (offset + 1 >= data.size())
        return std::nullopt;
    if (!data[offset].has_value() || !data[offset + 1].has_value())
        return std::nullopt;
    const uint16_t high = std::get<uint16_t>(data[offset].value());
    const uint16_t low = std::get<uint16_t>(data[offset + 1].value());

    return static_cast<uint16_t>((high << 8) | low);
}

StoredData PagingAllocator::readFromFrame(const int frameNumber, const int offset) const {
    std::lock_guard lock(pagingMutex);
    const auto data = frameTable[frameNumber].data[offset];

    if (data.has_value()) {
        if (std::holds_alternative<uint16_t>(data.value())) {
            auto valOpt = readUint16FromFrame(frameNumber, offset);

            if (valOpt)
                return StoredData(*valOpt);
        }

        return data.value();
    }

    throw std::runtime_error("Tried accessing 2nd byte - Possible misaligned address");
}
void PagingAllocator::writeToFrame(const int frameNumber, const int offset, const uint16_t data) {
    std::lock_guard lock(pagingMutex);

    const uint8_t highByte = static_cast<uint8_t>((data >> 8) & 0xFF);
    const uint8_t lowByte = static_cast<uint8_t>(data & 0xFF);

    frameTable[frameNumber].data[offset] = StoredData{static_cast<uint16_t>(highByte)};
    frameTable[frameNumber].data[offset + 1] = StoredData{static_cast<uint16_t>(lowByte)};
}

PagingAllocator::PagingAllocator() {
    const auto overallMem = Config::getInstance().getMaxOverallMem();
    const auto frameSize = Config::getInstance().getMemPerFrame();

    this->totalFrames = overallMem / frameSize;
    frameTable.resize(totalFrames);

    for (size_t i = 0; i < totalFrames; ++i) {
        freeFrameIndices.push_back(i);
    }

    // If the backing store exists, clear it.
    std::ofstream backingStore(BACKING_STORE_FILE, std::ios::trunc);
}

int PagingAllocator::allocateFrame(const int pid, const int pageNumber,
                                   const std::vector<std::optional<StoredData>>& pageData) {
    if (freeFrameIndices.empty()) {
        return -1;  // Signal: no frame available
    }

    const int frameIndex = freeFrameIndices.front();
    freeFrameIndices.pop_front();

    frameTable[frameIndex] = {pid, pageNumber, pageData};
    oldFramesQueue.push_back(frameIndex);

    ++allocatedFrames;

    return frameIndex;
}

int PagingAllocator::evictVictimFrame() {
    const int victimFrame = getVictimFrame();

    auto& [victimPID, victimPageNum, _] = frameTable[victimFrame];

    // Update victim's page table
    const auto victimProcess = ConsoleManager::getInstance().getProcessByPID(victimPID);

    if (victimProcess) {
        victimProcess->swapPageOut(victimPageNum);
    } else {
        const auto error = std::format("Tried swapping out invalid process with PID {}", victimPID);
        throw std::runtime_error(error);
    }

    // Write frame to backing store
    swapOut(victimFrame);

    return victimFrame;
}

int PagingAllocator::getVictimFrame() {
    if (oldFramesQueue.empty()) {
        throw std::runtime_error("No frames available for replacement.");
    }

    const int victimIndex = oldFramesQueue.front();
    oldFramesQueue.pop_front();

    return victimIndex;
}

void PagingAllocator::freeFrame(const int frameIndex) {
    frameTable[frameIndex] = FrameInfo{};
    freeFrameIndices.push_back(frameIndex);

    // Remove the frame from the old frames queue
    std::erase(oldFramesQueue, frameIndex);

    --allocatedFrames;
}

void PagingAllocator::swapOut(const int frameIndex) {
    if (frameIndex < 0 || frameIndex >= static_cast<int>(frameTable.size())) {
        throw std::out_of_range("Invalid frame index for swapOut.");
    }

    const auto& [pid, pageNumber, _] = frameTable[frameIndex];

    // Lookup the owning process
    const auto process = ConsoleManager::getInstance().getProcessByPID(pid);
    if (!process) {
        throw std::runtime_error("Process not found during swapOut.");
    }

    // Append to the backing store file
    std::ofstream backingFile(BACKING_STORE_FILE, std::ios::app);
    if (!backingFile) {
        throw std::runtime_error("Failed to open backing store for writing.");
    }
    backingFile << pid << " " << pageNumber << "\n";

    for (const auto& entry : frameTable[frameIndex].data) {
        if (!entry.has_value())
            continue;

        if (std::holds_alternative<uint16_t>(entry.value())) {
            const auto val = std::get<uint16_t>(entry.value());
            std::print(backingFile, "VAL {}\n", val);
        } else {
            const auto& instr = std::get<std::shared_ptr<Instruction>>(entry.value());
            std::print(backingFile, "{}\n", instr->serialize());
        }
    }

    // Update process page table
    process->swapPageOut(pageNumber);
    this->numPagedOut += 1;

    // Free the frame
    freeFrame(frameIndex);
}

std::vector<std::optional<StoredData>> PagingAllocator::swapIn(int pid, int pageNumber) const {
    std::ifstream backingFile(BACKING_STORE_FILE);
    std::ofstream tempFile("temp.txt");

    if (!backingFile.is_open()) {
        throw std::runtime_error("Failed to open backing store file.");
    }
    if (!tempFile.is_open()) {
        throw std::runtime_error("Failed to open temporary backing store file.");
    }

    std::vector<std::optional<StoredData>> storedData;
    std::string line;
    bool inTargetBlock = false;

    while (std::getline(backingFile, line)) {
        // Check if this is a header line
        std::istringstream iss(line);
        int readPID, readPage;
        if ((iss >> readPID >> readPage)) {
            // If we were reading the target block, stop here
            if (inTargetBlock) {
                tempFile << line << "\n";
                inTargetBlock = false;
                continue;
            }

            // Found the header we want
            if (readPID == pid && readPage == pageNumber) {
                inTargetBlock = true;
                continue;  // don't write this header
            }
        }

        if (inTargetBlock) {
            // This is a data line
            if (line.starts_with("VAL ")) {
                uint16_t val = std::stoi(line.substr(4));
                uint8_t highByte = static_cast<uint8_t>((val >> 8) & 0xFF);
                uint8_t lowByte = static_cast<uint8_t>(val & 0xFF);

                storedData.push_back(highByte);
                storedData.push_back(lowByte);
            } else if (!line.empty()) {
                std::string instructionBlock = line;

                std::istringstream instrStream(line);
                auto decodedInstr = InstructionFactory::deserializeInstruction(instrStream);

                storedData.push_back(decodedInstr);
                storedData.push_back(std::nullopt);
            }
        } else {
            tempFile << line << "\n";
        }
    }

    backingFile.close();
    tempFile.close();

    std::remove(BACKING_STORE_FILE);
    std::rename("temp.txt", BACKING_STORE_FILE);

    return storedData;
}
