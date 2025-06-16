#include "Instruction.h"

Instruction::Instruction(int lines, std::shared_ptr<Process> process)
    : lineCount(lines), process(std::move(process)) {
}

int Instruction::getLineCount() const noexcept {
    return lineCount;
}