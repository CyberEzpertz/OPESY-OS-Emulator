#include "WriteInstruction.h"

#include <format>

#include "Process.h"

WriteInstruction::WriteInstruction(const int address, const uint16_t value, const int pid)
    : Instruction(1, pid), address(address), value(value) {
    this->opCode = "WRITE";
    if (pid < 0) {
        throw std::runtime_error("BRUH");
    }
}

WriteInstruction::WriteInstruction(const int address, const std::string &varName, const int pid)
    : Instruction(1, pid), address(address), value(0), varName(varName), hasVar(true) {
    this->opCode = "WRITE";
    if (pid < 0) {
        throw std::runtime_error("BRUH");
    }
}

void WriteInstruction::execute() {
    const auto proc = getProcess();

    if (hasVar && !varName.empty()) {
        value = proc->getVariable(varName);
    }

    proc->writeToHeap(address, value);
}

std::string WriteInstruction::serialize() const {
    std::string valueStr = hasVar ? varName : std::to_string(value);
    return std::format("WRITE {} {} {} {}", hasVar, address, valueStr, pid);
}
