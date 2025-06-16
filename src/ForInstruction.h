#pragma once
#include <memory>
#include <vector>

#include "Instruction.h"

class ForInstruction final : public Instruction {
public:
    void execute() override;
    ForInstruction(
        const int pid, int totalLoops,
        const std::vector<std::shared_ptr<Instruction>> &instructions);

    bool isComplete() const override;

private:
    int totalLoops;
    int currentLoop;

    int nestLevel = 1;
    int currentInstructIdx;
    int totalInstructions;

    std::vector<std::shared_ptr<Instruction>> instructions;
};
