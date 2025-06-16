#pragma once
#include "Instruction.h"

class DeclareInstruction final : public Instruction {
public:
    DeclareInstruction(const std::string& name, const uint16_t value,
                       const std::shared_ptr<Process>& process);
    void execute() override;

private:
    std::string name;
    uint16_t value;
};
