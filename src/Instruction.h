#pragma once

#include <string>
#include <vector>
#include <memory>
#include <string_view>
#include <concepts>

// Forward declaration for process logs
class Process {
public:
    std::vector<std::string> logs;
};

// Interface class for all instructions
class Instruction {
protected:
    int lineCount;

public:
    // Constructor - default line count is 1 for most instructions
    explicit Instruction(int lines = 1) : lineCount(lines) {}

    // Virtual destructor for proper cleanup
    virtual ~Instruction() = default;

    // Delete copy constructor and assignment operator (modern C++ best practice)
    Instruction(const Instruction&) = delete;
    Instruction& operator=(const Instruction&) = delete;

    // Default move constructor and assignment operator
    Instruction(Instruction&&) = default;
    Instruction& operator=(Instruction&&) = default;

    // Pure virtual execute method to be implemented by all inheriting classes
    virtual void execute(Process& process) = 0;

    // Getter for line count (constexpr for compile-time evaluation when possible)
    [[nodiscard]] constexpr int getLineCount() const noexcept {
        return lineCount;
    }
};

// PrintInstruction class that inherits from Instruction
class PrintInstruction final : public Instruction {
private:
    std::string message;

public:
    // Constructor takes a string message to print - using string_view for efficiency
    template<typename T>
    requires std::convertible_to<T, std::string_view>
    explicit PrintInstruction(T&& msg) : Instruction(1), message(std::forward<T>(msg)) {}

    // Implementation of execute method
    void execute(Process& process) override {
        // Instead of printing to console, add the message to process logs
        process.logs.emplace_back(message);
    }

    // Getter for the message (optional utility method)
    [[nodiscard]] const std::string& getMessage() const noexcept {
        return message;
    }
};