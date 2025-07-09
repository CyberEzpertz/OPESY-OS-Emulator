#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "IMemoryAllocator.h"
#include "Screen.h"

class FlatMemoryAllocator : public IMemoryAllocator {
public:
    static FlatMemoryAllocator& getInstance();

    // Deleted copy/move constructors and assignment
    FlatMemoryAllocator(const FlatMemoryAllocator&) = delete;
    FlatMemoryAllocator& operator=(const FlatMemoryAllocator&) = delete;
    FlatMemoryAllocator(FlatMemoryAllocator&&) = delete;
    FlatMemoryAllocator& operator=(FlatMemoryAllocator&&) = delete;

    // IMemoryAllocator interface
    void* allocate(size_t size, const std::string& processName, std::shared_ptr<Screen> process) override;
    void deallocate(void* ptr, std::shared_ptr<Screen> process) override;
    void* getMemoryPtr(const std::string& processName) override;
    size_t getProcessMemoryUsage(const std::string& processName) const override;
    size_t getTotalMemoryUsage() const override;
    void visualizeMemoryASCII(int quantumCycle) override;

    // Getters
    size_t getAllocatedSize() const;
    size_t getMaximumSize() const;

private:
    FlatMemoryAllocator();

    size_t maximumSize;
    size_t allocatedSize;
    std::vector<char> memory;
    std::unordered_map<size_t, std::string> allocationMap;
    std::mutex allocationMapMutex;

    bool canAllocateAt(size_t index, size_t size);
    void allocateAt(size_t index, size_t size, const std::string& processName);
    void deallocateAt(size_t index, size_t size);
};
