#pragma once

#include <memory>

class Process;

class Instruction {
protected:
    int lineCount;
    int pid;

    std::shared_ptr<Process> getProcess() const;

public:
    explicit Instruction(int lines, int pid);

    virtual ~Instruction() = default;

    Instruction(const Instruction&) = delete;
    Instruction& operator=(const Instruction&) = delete;

    Instruction(Instruction&&) = default;
    Instruction& operator=(Instruction&&) = default;

    // Instructions will generally be one-liners, except for the for loops.
    virtual bool isComplete() const {
        return true;
    }

    virtual void execute() = 0;

    [[nodiscard]] virtual int getLineCount() const noexcept;
};
