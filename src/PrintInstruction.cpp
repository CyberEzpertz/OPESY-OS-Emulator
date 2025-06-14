#include "PrintInstruction.h"

#include <string>

#include "Process.h"

PrintInstruction::PrintInstruction(const std::string& msg,
                                   std::shared_ptr<Process> process)
    : Instruction(1, std::move(process)), message(msg) {
}

void PrintInstruction::execute() {
    std::string printMessage =
        "(" + process->getTimestamp() + ")" +
        " Core:" + std::to_string(process->getCurrentCore()) + " \"" + message +
        "\"";

    process->getLogs().emplace_back(std::move(printMessage));
}

const std::string& PrintInstruction::getMessage() const noexcept {
    return message;
}