#pragma once

#include <memory>
#include <string>

#include "Screen.h"

class Process;
class IMemoryAllocator {
public:
    virtual ~IMemoryAllocator() = default;

    // Tries to allocate memory for the given process.
    // Returns a pointer to the allocated memory or nullptr if allocation fails.
    virtual void* allocate(size_t size, const std::string& processName, std::shared_ptr<Process> process) = 0;

    // Frees the memory previously assigned to the process.
    virtual void deallocate(void* ptr, std::shared_ptr<Process> process) = 0;

    // Returns the memory usage of a specific process (0 if not allocated).
    [[nodiscard]] virtual size_t getProcessMemoryUsage(const std::string& processName) const = 0;

    // Returns the total amount of memory currently allocated.
    [[nodiscard]] virtual size_t getTotalMemoryUsage() const = 0;

    // Writes the current memory layout to memory_stamp_<quantumCycle>.txt,
    // displaying ranges for each process and free segment.
    virtual void visualizeMemory(int quantumCycle) = 0;
};
