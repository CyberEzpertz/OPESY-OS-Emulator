#pragma once

#include <memory>

class Process;

class Instruction {
protected:
    int lineCount;
    std::shared_ptr<Process> process;

public:
    explicit Instruction(int lines, std::shared_ptr<Process> process);

    virtual ~Instruction() = default;

    Instruction(const Instruction&) = delete;
    Instruction& operator=(const Instruction&) = delete;

    Instruction(Instruction&&) = default;
    Instruction& operator=(Instruction&&) = default;

    virtual void execute() = 0;

    [[nodiscard]] int getLineCount() const noexcept;
};
