//
// Created by Jan on 26/07/2025.
//

#include "PagingAllocator.h"

#include <fstream>
#include <print>

#include "Config.h"
#include "ConsoleManager.h"
#include "Process.h"

static constexpr auto BACKING_STORE_FILE = "csopesy-backing-store.txt";

PagingAllocator& PagingAllocator::getInstance() {
    static auto* instance = new PagingAllocator();
    return *instance;
}

void PagingAllocator::handlePageFault(const int pid, const int pageNumber) {
    const auto process = ConsoleManager::getInstance().getProcessByPID(pid);

    std::lock_guard lock(pagingMutex);

    const PageEntry entry = process->getPageEntry(pageNumber);

    // If the entry was swapped out, this will trigger
    if (entry.inBackingStore) {
        swapIn(pid, pageNumber);
    }

    int frameIndex = allocateFrame(pid, pageNumber);

    // If the allocation was unsuccessful, evict a frame then allocate it again
    if (frameIndex == -1) {
        // Will free up a frame and put it in the free frame list
        evictVictimFrame();

        // This should now succeed since we evicted a frame
        frameIndex = allocateFrame(pid, pageNumber);

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
    std::string backingStoreFile = BACKING_STORE_FILE;

    std::ifstream inFile(backingStoreFile);
    std::ofstream outFile("temp.txt");

    std::string line;
    while (std::getline(inFile, line)) {
        std::istringstream iss(line);
        int filePid, pageNumber;

        if (iss >> filePid >> pageNumber) {
            if (filePid != pid) {
                outFile << line << '\n';
            }
        }
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
        const auto& [pid, pageNumber] = frameTable[i];
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
int PagingAllocator::getFreeMemory() const {
    return Config::getInstance().getMaxOverallMem() - getUsedMemory();
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

int PagingAllocator::allocateFrame(const int pid, const int pageNumber) {
    if (freeFrameIndices.empty()) {
        return -1;  // Signal: no frame available
    }

    const int frameIndex = freeFrameIndices.front();
    freeFrameIndices.pop_front();

    frameTable[frameIndex] = {pid, pageNumber};
    oldFramesQueue.push_back(frameIndex);

    ++allocatedFrames;

    return frameIndex;
}

int PagingAllocator::evictVictimFrame() {
    const int victimFrame = getVictimFrame();

    auto& [victimPID, victimPageNum] = frameTable[victimFrame];

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

    const auto& [pid, pageNumber] = frameTable[frameIndex];

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

    // Update process page table
    process->swapPageOut(pageNumber);
    this->numPagedOut += 1;

    // Free the frame
    freeFrame(frameIndex);
}

void PagingAllocator::swapIn(const int pid, int pageNumber) const {
    // Read the previous contents
    std::ifstream backingFile(BACKING_STORE_FILE);

    // Will replace the old backing store
    std::ofstream tempFile("temp.txt");

    std::string line;
    while (std::getline(backingFile, line)) {
        std::istringstream iss(line);
        int readPID, page;
        if (!(iss >> readPID >> page))
            throw std::runtime_error("Backing store file is malformed, please check it.");

        // Skip the page to be loaded back in
        if (readPID == pid && page == pageNumber) {
            continue;
        }

        tempFile << line << "\n";  // Keep all other lines
    }

    backingFile.close();
    tempFile.close();

    // 3. Replace the backing store with the temp file
    std::remove(BACKING_STORE_FILE);
    std::rename("temp.txt", BACKING_STORE_FILE);
}
