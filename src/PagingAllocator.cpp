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

PageFaultResult PagingAllocator::handlePageFault(const int pid, const int pageNumber) {
    constexpr int maxAttempts = 10;
    int attempts = 0;

    auto process = ConsoleManager::getInstance().getProcessByPID(pid);

    // Load page data only once
    std::vector<std::optional<StoredData>> pageData;
    {
        std::lock_guard lock(pagingMutex);
        const PageEntry entry = process->getPageEntry(pageNumber);
        pageData = entry.inBackingStore ? swapIn(pid, pageNumber) : process->getPageData(pageNumber);
    }

    while (true) {
        std::lock_guard lock(pagingMutex);

        int frameIndex = allocateFrame(pid, pageNumber, pageData);
        if (frameIndex != -1) {
            process->swapPageIn(pageNumber, frameIndex);
            ++numPagedIn;
            return SUCCESS;
        }

        if (!evictVictimFrame()) {
            ++attempts;
            std::this_thread::yield();
            continue;
        }

        // Retry allocation after eviction
        frameIndex = allocateFrame(pid, pageNumber, pageData);
        if (frameIndex == -1) {
            throw std::runtime_error("Failed to allocate frame after successful eviction");
        }

        process->swapPageIn(pageNumber, frameIndex);
        ++numPagedIn;
        return SUCCESS;
    }

    return DEFERRED;
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
        const auto& [pid, pageNumber, _, _pinned] = frameTable[i];
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
bool PagingAllocator::pinFrame(const int frameNumber, const int pid, const int pageNumber) {
    std::lock_guard lock(pagingMutex);
    const auto frame = frameTable[frameNumber];

    if (frame.pid != pid || frame.pageNumber != pageNumber)
        return false;

    frameTable[frameNumber].isPinned = true;
    return true;
}

StoredData PagingAllocator::readFromFrame(const int frameNumber, const int offset) {
    std::lock_guard lock(pagingMutex);
    const auto data = frameTable[frameNumber].data[offset];

    if (data.has_value()) {
        frameTable[frameNumber].isPinned = false;
        if (std::holds_alternative<uint16_t>(data.value())) {
            auto valOpt = readUint16FromFrame(frameNumber, offset);

            if (valOpt) {
                return {*valOpt};
            }
        }

        return data.value();
    }

    throw std::runtime_error("Tried accessing 2nd byte - Possible misaligned address");
}
void PagingAllocator::writeToFrame(const int frameNumber, const int offset, const uint16_t data) {
    std::lock_guard lock(pagingMutex);
    frameTable[frameNumber].isPinned = false;

    const auto highByte = static_cast<uint8_t>((data >> 8) & 0xFF);
    const auto lowByte = static_cast<uint8_t>(data & 0xFF);

    frameTable[frameNumber].data[offset] = StoredData{static_cast<uint16_t>(highByte)};
    frameTable[frameNumber].data[offset + 1] = StoredData{static_cast<uint16_t>(lowByte)};
}

PagingAllocator::PagingAllocator() {
    const auto overallMem = Config::getInstance().getMaxOverallMem();
    const auto frameSize = Config::getInstance().getMemPerFrame();

    this->totalFrames = overallMem / frameSize;
    frameTable.resize(totalFrames);

    for (int i = 0; i < totalFrames; ++i) {
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

    frameTable[frameIndex] = {pid, pageNumber, pageData, true};
    oldFramesQueue.push_back(frameIndex);

    ++allocatedFrames;

    return frameIndex;
}

bool PagingAllocator::evictVictimFrame() {
    const int victimFrame = getVictimFrame();

    // Means no frames were available to be replaced
    if (victimFrame == -1)
        return false;

    auto& [victimPID, victimPageNum, _, _pinned] = frameTable[victimFrame];

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

    return true;
}

int PagingAllocator::getVictimFrame() {
    const size_t oldFrames = oldFramesQueue.size();
    for (size_t i = 0; i < oldFrames; ++i) {
        const int victimIndex = oldFramesQueue.front();
        oldFramesQueue.pop_front();

        if (!frameTable[victimIndex].isPinned) {
            return victimIndex;
        }

        oldFramesQueue.push_back(victimIndex);
    }

    return -1;
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

    const auto& [pid, pageNumber, _, _pinned] = frameTable[frameIndex];
    const auto process = ConsoleManager::getInstance().getProcessByPID(pid);
    if (!process) {
        throw std::runtime_error("Process not found during swapOut.");
    }

    std::ofstream backingFile(BACKING_STORE_FILE, std::ios::app);
    if (!backingFile) {
        throw std::runtime_error("Failed to open backing store for writing.");
    }

    backingFile << pid << " " << pageNumber << "\n";

    const auto& data = frameTable[frameIndex].data;
    const int memSize = static_cast<int>(data.size());

    int i = 0;
    while (i + 1 < memSize) {
        // Check for 2-byte VAL
        if (data[i].has_value() && data[i + 1].has_value() && std::holds_alternative<uint16_t>(data[i].value()) &&
            std::holds_alternative<uint16_t>(data[i + 1].value())) {
            const auto high = static_cast<uint8_t>(std::get<uint16_t>(data[i].value()));
            const auto low = static_cast<uint8_t>(std::get<uint16_t>(data[i + 1].value()));
            const uint16_t combined = (static_cast<uint16_t>(high) << 8) | low;

            int start = i;
            int count = 1;
            i += 2;

            // Compress repeated VALs
            while (i + 1 < memSize && data[i].has_value() && data[i + 1].has_value() &&
                   std::holds_alternative<uint16_t>(data[i].value()) &&
                   std::holds_alternative<uint16_t>(data[i + 1].value())) {
                const auto nextHigh = static_cast<uint8_t>(std::get<uint16_t>(data[i].value()));
                const auto nextLow = static_cast<uint8_t>(std::get<uint16_t>(data[i + 1].value()));
                const uint16_t nextCombined = (static_cast<uint16_t>(nextHigh) << 8) | nextLow;

                if (nextCombined == combined) {
                    count++;
                    i += 2;
                } else {
                    break;
                }
            }

            backingFile << "V " << start << " " << combined;
            if (count > 1) {
                backingFile << " x" << count;
            }
            backingFile << "\n";
        } else if (data[i].has_value() && std::holds_alternative<std::shared_ptr<Instruction>>(data[i].value())) {
            const auto& instr = std::get<std::shared_ptr<Instruction>>(data[i].value());
            backingFile << "I " << i << " " << instr->serialize() << "\n";
            ++i;
        } else {
            ++i;
        }
    }

    process->swapPageOut(pageNumber);
    this->numPagedOut += 1;
    freeFrame(frameIndex);
}
std::vector<std::optional<StoredData>> PagingAllocator::swapIn(int pid, int pageNumber) const {
    std::ifstream backingFile(BACKING_STORE_FILE);
    if (!backingFile.is_open())
        throw std::runtime_error("Failed to open backing store file.");

    std::vector<std::optional<StoredData>> storedData(Config::getInstance().getMemPerFrame(), std::nullopt);
    std::string line;
    bool inTargetBlock = false;

    while (std::getline(backingFile, line)) {
        std::istringstream iss(line);
        int readPID, readPage;

        if ((iss >> readPID >> readPage)) {
            inTargetBlock = (readPID == pid && readPage == pageNumber);
            continue;
        }

        if (!inTargetBlock)
            continue;

        if (line.starts_with("V")) {
            std::string tag;
            int offset;
            uint16_t value;
            size_t xIndex = line.find(" x");

            int count = 1;
            if (xIndex != std::string::npos) {
                std::string prefix = line.substr(0, xIndex);
                std::string suffix = line.substr(xIndex + 2);
                std::istringstream(prefix) >> tag >> offset >> value;
                count = std::stoi(suffix);
            } else {
                iss.clear();
                iss.str(line);
                iss >> tag >> offset >> value;
            }

            for (int i = 0; i < count; ++i) {
                int addr = offset + (i * 2);
                if (addr + 1 < static_cast<int>(storedData.size())) {
                    auto high = static_cast<uint8_t>((value >> 8) & 0xFF);
                    auto low = static_cast<uint8_t>(value & 0xFF);
                    storedData[addr] = static_cast<uint16_t>(high);
                    storedData[addr + 1] = static_cast<uint16_t>(low);
                }
            }
        } else if (line.starts_with("I")) {
            std::string tag;
            int offset;
            iss.clear();
            iss.str(line);
            iss >> tag >> offset;

            std::string serializedInstr = line.substr(line.find_first_of(" \t", 2) + 1);
            std::istringstream instrStream(serializedInstr);
            auto instr = InstructionFactory::deserializeInstruction(instrStream);

            if (offset >= 0 && offset < static_cast<int>(storedData.size())) {
                storedData[offset] = instr;
            }
        }
    }

    return storedData;
}
