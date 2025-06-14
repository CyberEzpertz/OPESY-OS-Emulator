#pragma once

#include <concepts>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Process.h"

class Instruction {
protected:
    int lineCount;
    Process* process;

public:
    explicit Instruction(int lines = 1, const Process* process = nullptr);

    virtual ~Instruction() = default;

    Instruction(const Instruction&) = delete;
    Instruction& operator=(const Instruction&) = delete;

    Instruction(Instruction&&) = default;
    Instruction& operator=(Instruction&&) = default;

    virtual void execute() = 0;

    [[nodiscard]] constexpr int getLineCount() const noexcept;
};