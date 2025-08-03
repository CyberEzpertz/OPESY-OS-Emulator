#pragma once
#include "Instruction.h"
class SleepInstruction final : public Instruction {
public:
    SleepInstruction(uint8_t ticks, const int pid);

private:
    uint8_t ticks;
    void execute() override;
    std::string serialize() const override;
};
