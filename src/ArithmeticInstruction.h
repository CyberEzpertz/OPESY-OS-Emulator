#pragma once
#include <variant>

#include "Instruction.h"

enum Operation { ADD, SUBTRACT };
using Operand = std::variant<std::string, uint16_t>;

class ArithmeticInstruction final : public Instruction {
public:
    ArithmeticInstruction(const std::string& resultName, const Operand& lhsVar,
                          const Operand& rhsVar, const Operation& operation,
                          const std::shared_ptr<Process>& process);

    void execute() override;
    uint16_t resolveOperand(const Operand& op) const;

private:
    Operation operation;
    std::string resultName;
    Operand lhsVar;
    Operand rhsVar;
};
