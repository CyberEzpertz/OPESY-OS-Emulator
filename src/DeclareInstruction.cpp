#include "DeclareInstruction.h"

#include "Process.h"
DeclareInstruction::DeclareInstruction(const std::string& name,
                                       const uint16_t value,
                                       const std::shared_ptr<Process>& process)
    : Instruction(1, process), name(name), value(value) {
}

void DeclareInstruction::execute() {
    process->setVariable(name, value);
}