#include "PrintInstruction.h"

void PrintInstruction::execute() {
    process->getLogs().emplace_back(message);
}

const std::string& PrintInstruction::getMessage() const noexcept {
    return message;
}