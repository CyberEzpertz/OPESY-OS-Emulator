#include "PrintInstruction.h"

#include <print>
#include <string>

#include "Process.h"

PrintInstruction::PrintInstruction(const std::string& msg,
                                   const std::shared_ptr<Process>& process)
    : Instruction(1, process), message(msg), varName("") {
}

PrintInstruction::PrintInstruction(const std::string& msg,
                                   const std::shared_ptr<Process>& process,
                                   const std::string& varName)
    : Instruction(1, process), message(msg), varName(varName) {
}

void PrintInstruction::execute() {
    if (!process) {
        std::println("Error: Tried to print, but no process attached.");
    }
    std::string varValue =
        varName != "" ? std::to_string(process->getVariable(varName)) : "";

    const std::string logMessage =
        std::format("({}) Core:{} \"{}{}\"", process->getTimestamp(),
                    process->getCurrentCore(), message, varValue);

    process->log(logMessage);
}

const std::string& PrintInstruction::getMessage() const noexcept {
    return message;
}