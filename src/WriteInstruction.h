#pragma once
#include "Instruction.h"

class WriteInstruction final : public Instruction {
public:
    WriteInstruction(int address, uint16_t value, int pid);
    WriteInstruction(int address, const std::string &varName, int pid);
    void execute() override;
    std::string serialize() const override;

private:
    int address;
    uint16_t value;
    std::string varName;
    bool hasVar = false;
};
