#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

#include "IMemoryAllocator.h"

class Process;
class FlatMemoryAllocator : public IMemoryAllocator {
public:
    static FlatMemoryAllocator& getInstance();

    // Deleted copy/move constructors and assignment
    FlatMemoryAllocator(const FlatMemoryAllocator&) = delete;
    FlatMemoryAllocator& operator=(const FlatMemoryAllocator&) = delete;
    FlatMemoryAllocator(FlatMemoryAllocator&&) = delete;
    FlatMemoryAllocator& operator=(FlatMemoryAllocator&&) = delete;

    // IMemoryAllocator interface
    void* allocate(size_t size, std::shared_ptr<Process> process) override;
    void deallocate(void* ptr, std::shared_ptr<Process> process) override;
    size_t getProcessMemoryUsage(const std::string& processName) const override;
    size_t getTotalMemoryUsage() const override;
    void visualizeMemory(int quantumCycle) override;

    // Getters
    size_t getMaximumMemory() const;

private:
    FlatMemoryAllocator();

    size_t maximumSize;
    size_t allocatedSize;
    std::vector<char> memoryView;
    std::vector<std::string> memoryMap;
    mutable std::shared_mutex memoryMutex;

    [[nodiscard]] bool canAllocateAt(size_t index, size_t size) const;
    void allocateAt(size_t index, size_t size, const std::string& processName);
    void deallocateAt(size_t index, size_t size);
};
