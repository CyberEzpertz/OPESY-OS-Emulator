#pragma once

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "IMemoryAllocator.h"

class Process;

class PagingAllocator : IMemoryAllocator {
public:
    static PagingAllocator& getInstance();

    PagingAllocator(const PagingAllocator&) = delete;
    PagingAllocator& operator=(const PagingAllocator&) = delete;
    PagingAllocator(PagingAllocator&&) = delete;
    PagingAllocator& operator=(PagingAllocator&&) = delete;

    // Initializes memory frame table
    void initialize();

    // Allocates virtual pages for a process (no physical frame yet)
    void allocatePages(std::shared_ptr<Process> process);

    // Handles a page fault by allocating a physical frame to the given virtual address
    void handlePageFault(std::shared_ptr<Process> process, uint16_t virtualAddress);

    // Frees all memory (physical and virtual) associated with a process
    void deallocate(void* ptr, std::shared_ptr<Process> process) override;

    void* allocate(size_t size, std::shared_ptr<Process> process) override;

    // Gets total physical memory usage in bytes
    [[nodiscard]] size_t getTotalMemoryUsage() const override;

    // Gets how many frames are used by a process
    [[nodiscard]] size_t getProcessMemoryUsage(const std::string& processName) const override;

    // Visualize memory as frame map with process and page info
    void visualizeMemory(int quantumCycle) override;

private:
    PagingAllocator();

    int allocateFrame(int pid, int pageNumber);
    void freeFrame(int frameIndex);

    void swapOut(int frameIndex);
    void swapIn(std::shared_ptr<Process> process, int pageNumber);

    // Frame table metadata
    struct FrameInfo {
        int pid = -1;
        int pageNumber = -1;
    };

    size_t totalFrames;
    size_t frameSize;

    std::vector<FrameInfo> frameTable;
    std::deque<int> freeFrameIndices;
};
