#pragma once
#include "Instruction.h"

class DeclareInstruction final : public Instruction {
public:
    DeclareInstruction(const std::string& name, uint16_t value, int pid);
    void execute() override;

private:
    std::string name;
    uint16_t value;
};
