#include "ForInstruction.h"

ForInstruction::ForInstruction(
    const std::shared_ptr<Process> &process, const int totalLoops,
    const std::vector<std::shared_ptr<Instruction>> &instructions)
    : Instruction(0, process),
      totalLoops(totalLoops),
      currentLoop(0),
      currentInstructIdx(0),
      instructions(instructions) {
    totalInstructions = instructions.size();

    int totalLineCount = 0;
    for (const auto &line : instructions) {
        totalLineCount += line->getLineCount();
    }

    // We add 1 because the for itself is a line of code
    totalLines = totalLineCount + 1;
}

void ForInstruction::execute() {
    if (currentInstructIdx >= instructions.size() || currentLoop >= totalLoops)
        return;

    const auto &currentInstruction = instructions[currentInstructIdx];
    currentInstruction->execute();

    if (currentInstruction->isComplete()) {
        currentInstructIdx = (currentInstructIdx + 1) % instructions.size();

        // Check if it's 0, if it is that means we've looped back.
        currentLoop += currentInstructIdx == 0 ? 1 : 0;
    }
}

bool ForInstruction::isComplete() const {
    return currentLoop >= totalLoops;
}