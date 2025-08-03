#pragma once
#include "Instruction.h"

class WriteInstruction final : public Instruction {
public:
    WriteInstruction(int address, uint16_t value, int pid);
    void execute() override;
    std::string serialize() const override;

private:
    int address;
    uint16_t value;
};
