#pragma once
#include "Instruction.h"
class SleepInstruction final : public Instruction {
public:
    SleepInstruction(uint8_t ticks, const std::shared_ptr<Process>& process);

private:
    uint8_t ticks;
    void execute() override;
};
