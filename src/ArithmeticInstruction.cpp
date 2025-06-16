#include "ArithmeticInstruction.h"

#include "Process.h"

ArithmeticInstruction::ArithmeticInstruction(
    const std::string& resultName, const Operand& lhsVar, const Operand& rhsVar,
    const Operation& operation, const std::shared_ptr<Process>& process)
    : Instruction(1, process),
      operation(operation),
      resultName(resultName),
      lhsVar(lhsVar),
      rhsVar(rhsVar) {
}
void ArithmeticInstruction::execute() {
    const uint16_t lhsValue = resolveOperand(lhsVar);
    const uint16_t rhsValue = resolveOperand(rhsVar);

    if (operation == ADD) {
        const uint32_t sum =
            static_cast<uint32_t>(lhsValue) + static_cast<uint32_t>(rhsValue);

        // Need to clamp the value
        const uint16_t result =
            sum > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(sum);

        process->setVariable(resultName, result);
    } else {
        // Need to clamp the value
        const uint16_t result = lhsValue < rhsValue ? 0 : lhsValue - rhsValue;
        process->setVariable(resultName, result);
    }
}

uint16_t ArithmeticInstruction::resolveOperand(const Operand& op) const {
    if (std::holds_alternative<std::string>(op)) {
        const auto& varName = std::get<std::string>(op);
        const auto value = process->getVariable(varName);

        return value;
    } else {
        return std::get<uint16_t>(op);
    }
}
