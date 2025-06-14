#include "PrintInstruction.h"

#include <string>

#include "Process.h"

PrintInstruction::PrintInstruction(const std::string& msg,
                                   std::shared_ptr<Process> process)
    : Instruction(1, std::move(process)), message(msg) {
}

void PrintInstruction::execute() {
    std::string logMessage =
        "(" + process->getTimestamp() + ")" +
        " Core:" + std::to_string(process->getCurrentCore()) + " \"" + message +
        "\"";

    process->log(logMessage);
}

const std::string& PrintInstruction::getMessage() const noexcept {
    return message;
}