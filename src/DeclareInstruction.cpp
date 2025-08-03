#include "DeclareInstruction.h"

#include "ConsoleManager.h"
#include "Process.h"
DeclareInstruction::DeclareInstruction(const std::string& name, const uint16_t value, const int pid)
    : Instruction(1, pid), name(name), value(value) {
}

void DeclareInstruction::execute() {
    const auto proc = getProcess();
    proc->setVariable(name, value);
}

std::string DeclareInstruction::serialize() const {
    return std::format("DECLARE {} {}", name, value);
}