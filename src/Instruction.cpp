#include "Instruction.h"

Instruction::Instruction(int lines, const Process* process) : lineCount(lines), process(const_cast<Process*>(process)) {
    // Constructor stores the number of lines and process pointer
}

int Instruction::getLineCount() const noexcept {
    return lineCount;
}