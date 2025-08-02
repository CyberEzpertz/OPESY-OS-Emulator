#include "WriteInstruction.h"

#include "Process.h"

WriteInstruction::WriteInstruction(int address, uint16_t value, int pid)
    : Instruction(1, pid), address(address), value(value) {
}

void WriteInstruction::execute() {
    const auto proc = getProcess();
    proc->writeToHeap(address, value);
}
