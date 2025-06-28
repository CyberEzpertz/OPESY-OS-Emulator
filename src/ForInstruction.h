#pragma once
#include <memory>
#include <vector>

#include "Instruction.h"

class ForInstruction final : public Instruction {
public:
    void execute() override;
    bool isComplete() const override;
    void restartCounters();
    ForInstruction(
        const int pid, int totalLoops,
        const std::vector<std::shared_ptr<Instruction>> &instructions);

private:
    int totalLoops;
    int currentLoop;

    int nestLevel = 1;
    int currentInstructIdx;

    std::vector<std::shared_ptr<Instruction>> instructions;
};
