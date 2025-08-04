#include "ReadInstruction.h"

#include <sstream>

#include "Process.h"

ReadInstruction::ReadInstruction(const std::string& variableName, int address, int pid)
    : Instruction(1, pid), variableName(variableName), address(address) {
    this->opCode = "READ";
}

void ReadInstruction::execute() {
    const auto proc = getProcess();
    uint16_t value = proc->readFromHeap(address);
    proc->declareVariable(variableName, value);
}

std::string ReadInstruction::serialize() const {
    return std::format("READ {} {} {}", variableName, address, pid);
}