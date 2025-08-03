#pragma once
#include <variant>

#include "Instruction.h"

enum Operation { ADD, SUBTRACT };
using Operand = std::variant<std::string, uint16_t>;

class ArithmeticInstruction final : public Instruction {
public:
    ArithmeticInstruction(const std::string& resultName, const Operand& lhsVar, const Operand& rhsVar,
                          const Operation& operation, const int pid);

    void execute() override;
    uint16_t resolveOperand(const Operand& op) const;
    std::string getOperandString(const Operand& operand) const;
    std::string serialize() const override;

private:
    Operation operation;
    std::string resultName;
    Operand lhsVar;
    Operand rhsVar;
};
