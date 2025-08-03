#include "ArithmeticInstruction.h"

#include "Process.h"

ArithmeticInstruction::ArithmeticInstruction(const std::string& resultName, const Operand& lhsVar,
                                             const Operand& rhsVar, const Operation& operation, const int pid)
    : Instruction(1, pid), operation(operation), resultName(resultName), lhsVar(lhsVar), rhsVar(rhsVar) {
}
void ArithmeticInstruction::execute() {
    const uint16_t lhsValue = resolveOperand(lhsVar);
    const uint16_t rhsValue = resolveOperand(rhsVar);
    const auto process = getProcess();

    if (operation == ADD) {
        const uint32_t sum = static_cast<uint32_t>(lhsValue) + static_cast<uint32_t>(rhsValue);

        // Need to clamp the value
        const uint16_t result = sum > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(sum);

        process->setVariable(resultName, result);
    } else {
        // Need to clamp the value
        const uint16_t result = lhsValue < rhsValue ? 0 : lhsValue - rhsValue;
        process->setVariable(resultName, result);
    }
}

uint16_t ArithmeticInstruction::resolveOperand(const Operand& op) const {
    const auto process = getProcess();

    if (std::holds_alternative<std::string>(op)) {
        const auto& varName = std::get<std::string>(op);
        const auto value = process->getVariable(varName);

        return value;
    } else {
        return std::get<uint16_t>(op);
    }
}

std::string ArithmeticInstruction::getOperandString(const Operand& operand) const {
    return std::visit(
        [](const auto& val) -> std::string {
            using T = std::decay_t<decltype(val)>;

            // If val is string, return as is
            if constexpr (std::is_same_v<T, std::string>)
                return val;

            // If val is int, convert it first
            else
                return std::to_string(val);
        },
        operand);
}

std::string ArithmeticInstruction::serialize() const {
    auto lhsStr = getOperandString(lhsVar);
    auto rhsStr = getOperandString(rhsVar);

    return std::format("ARITH {} {} {} {} {}", resultName, lhsStr, rhsStr, static_cast<int>(operation), pid);
}