#include "ForInstruction.h"

#include <print>

#include "Process.h"

ForInstruction::ForInstruction(const int pid, const int totalLoops,
                               const std::vector<std::shared_ptr<Instruction>> &instructions)
    : Instruction(0, pid), totalLoops(totalLoops), currentLoop(0), currentInstructIdx(0), instructions(instructions) {
    int totalLineCount = 0;
    for (const auto &line : instructions) {
        totalLineCount += line->getLineCount();
    }

    lineCount = totalLoops * totalLineCount;
}

void ForInstruction::execute() {
    if (currentInstructIdx >= instructions.size() || currentLoop >= totalLoops) {
        std::println("Tried executing for-loop beyond bounds.");
        return;
    }
    auto process = getProcess();

    const auto &currentInstruction = instructions[currentInstructIdx];
    currentInstruction->execute();

    if (currentInstruction->isComplete()) {
        currentInstructIdx = (currentInstructIdx + 1) % instructions.size();

        // Check if it's 0, if it is that means we've looped back.
        if (currentInstructIdx == 0) {
            currentLoop++;
        }
        // If it's a for-loop, we have to restart its counters for the next
        // iterations.
        if (const auto forInst = std::dynamic_pointer_cast<ForInstruction>(currentInstruction)) {
            forInst->restartCounters();
        }
    }
}

bool ForInstruction::isComplete() const {
    return currentLoop >= totalLoops;
}

void ForInstruction::restartCounters() {
    currentLoop = 0;
    currentInstructIdx = 0;
}