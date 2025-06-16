#include "Instruction.h"

#include "ConsoleManager.h"

std::shared_ptr<Process> Instruction::getProcess() const {
    const auto process = ConsoleManager::getInstance().getProcessByPID(pid);

    if (!process) {
        throw std::runtime_error(
            "Instruction tried to access invalid process.");
    }

    return process;
}
Instruction::Instruction(const int lines, const int pid)
    : lineCount(lines), pid(pid) {
}

int Instruction::getLineCount() const noexcept {
    return lineCount;
}