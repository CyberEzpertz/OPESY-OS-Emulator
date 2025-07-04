#include "PrintInstruction.h"

#include <print>
#include <string>

#include "ConsoleManager.h"
#include "Process.h"

PrintInstruction::PrintInstruction(const std::string& msg, const int pid)
    : Instruction(1, pid), message(msg), varName("") {
}

PrintInstruction::PrintInstruction(const std::string& msg, const int pid,
                                   const std::string& varName)
    : Instruction(1, pid), message(msg), varName(varName) {
}

void PrintInstruction::execute() {
    const auto process = getProcess();

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