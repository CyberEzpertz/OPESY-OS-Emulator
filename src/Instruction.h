#pragma once

#include <string>
#include <vector>
#include <memory>
#include <string_view>
#include <concepts>

class Process {
public:
    std::vector<std::string> logs;
};

class Instruction {
protected:
    int lineCount;

public:
    explicit Instruction(int lines = 1) : lineCount(lines) {}

    virtual ~Instruction() = default;

    Instruction(const Instruction&) = delete;
    Instruction& operator=(const Instruction&) = delete;

    Instruction(Instruction&&) = default;
    Instruction& operator=(Instruction&&) = default;

    virtual void execute(Process& process) = 0;

    [[nodiscard]] constexpr int getLineCount() const noexcept {
        return lineCount;
    }
};
