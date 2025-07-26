#pragma once

#include <deque>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

#include "IMemoryAllocator.h"

class Process;

class PagingAllocator {
public:
    static PagingAllocator& getInstance();

    PagingAllocator(const PagingAllocator&) = delete;
    PagingAllocator& operator=(const PagingAllocator&) = delete;
    PagingAllocator(PagingAllocator&&) = delete;
    PagingAllocator& operator=(PagingAllocator&&) = delete;

    // Handles a page fault by allocating a physical frame to the given virtual address
    void handlePageFault(int pid, int pageNumber);

    // Frees all memory (physical and virtual) associated with a process
    void deallocate(int pid);

    // Visualize memory as frame map with process and page info
    void visualizeMemory(int quantumCycle);

    int getUsedMemory() const;
    int getFreeMemory() const;

private:
    PagingAllocator();

    int allocateFrame(int pid, int pageNumber);
    int evictVictimFrame();
    int getVictimFrame();
    void freeFrame(int frameIndex);

    void swapOut(int frameIndex);
    void swapIn(int pid, int pageNumber) const;

    // Frame table metadata
    struct FrameInfo {
        int pid = -1;
        int pageNumber = -1;
    };

    size_t totalFrames;
    size_t allocatedFrames = 0;

    std::vector<FrameInfo> frameTable;
    std::deque<int> freeFrameIndices;
    std::deque<int> oldFramesQueue;

    std::atomic<int> numPagedIn = 0;
    std::atomic<int> numPagedOut = 0;

    mutable std::mutex pagingMutex;
};
