#pragma once

#include <atomic>
#include <deque>
#include <memory>
#include <shared_mutex>
#include <variant>
#include <vector>
#include <optional>

class Instruction;
class Process;

using StoredData = std::variant<uint16_t, std::shared_ptr<Instruction>>;

struct FrameInfo {
    int pid = -1;
    int pageNumber = -1;
    std::vector<std::optional<StoredData>> data;
    bool isPinned = false;
};

enum PageFaultResult { SUCCESS, DEFERRED };

class PagingAllocator {
public:
    static PagingAllocator& getInstance();

    PagingAllocator(const PagingAllocator&) = delete;
    PagingAllocator& operator=(const PagingAllocator&) = delete;
    PagingAllocator(PagingAllocator&&) = delete;
    PagingAllocator& operator=(PagingAllocator&&) = delete;

    // Handles a page fault by allocating a physical frame to the given virtual address
    PageFaultResult handlePageFault(int pid, int pageNumber);

    // Frees all memory (physical and virtual) associated with a process
    void deallocate(int pid);

    // Visualize memory as frame map with process and page info
    void visualizeMemory();

    int getUsedMemory() const;
    int getNumPagedIn() const;
    int getNumPagedOut() const;
    int getFreeMemory() const;
    std::optional<uint16_t> readUint16FromFrame(int frameNumber, int offset) const;
    bool pinFrame(int frameNumber, int pid, int pageNumber);

    StoredData readFromFrame(int frameNumber, int offset);
    void writeToFrame(int frameNumber, int offset, uint16_t data);

private:
    PagingAllocator();

    int allocateFrame(int pid, int pageNumber, const std::vector<std::optional<StoredData>>& pageData);
    bool evictVictimFrame();
    int getVictimFrame();
    void freeFrame(int frameIndex);

    void swapOut(int frameIndex);
    std::vector<std::optional<StoredData>> swapIn(int pid, int pageNumber) const;

    size_t totalFrames;
    std::atomic<size_t> allocatedFrames = 0;

    std::vector<FrameInfo> frameTable;
    std::deque<int> freeFrameIndices;
    std::deque<int> oldFramesQueue;

    std::atomic<int> numPagedIn = 0;
    std::atomic<int> numPagedOut = 0;

    mutable std::mutex pagingMutex;
};
