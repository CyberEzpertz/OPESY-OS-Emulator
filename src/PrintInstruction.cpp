#include "PrintInstruction.h"

#include <print>
#include <string>

#include "Process.h"

PrintInstruction::PrintInstruction(const std::string& msg,
                                   const std::shared_ptr<Process>& process)
    : Instruction(1, process), message(msg) {
}

void PrintInstruction::execute() {
    if (!process) {
        std::println("Error: Tried to print, but no process attached.");
    }

    const std::string logMessage =
        "(" + process->getTimestamp() + ")" +
        " Core:" + std::to_string(process->getCurrentCore()) + " \"" + message +
        "\"";

    process->log(logMessage);
}

const std::string& PrintInstruction::getMessage() const noexcept {
    return message;
}