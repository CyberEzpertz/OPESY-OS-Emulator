#pragma once
#include "Instruction.h"

class ReadInstruction final : public Instruction {
public:
    ReadInstruction(const std::string& variableName, int address, int pid);
    void execute() override;
    std::string serialize() const override;

private:
    std::string variableName;
    int address;
};
