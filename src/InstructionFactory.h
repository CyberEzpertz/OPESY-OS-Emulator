#pragma once

#include <memory>
#include <random>
#include <set>
#include <vector>

#include "Instruction.h"
#include "PrintInstruction.h"

class InstructionFactory {
public:
    static uint64_t calculateProcessMemoryRequirement(int numInstructions);
    static std::vector<std::shared_ptr<Instruction>> generateInstructions(int pid, const std::string& processName);
    static std::mt19937 rng;
    static int generateRandomNum(int min, int max);
    static std::vector<std::shared_ptr<Instruction>> createAlternatingPrintAdd(int pid);

private:
    static std::shared_ptr<Instruction> createRandomInstruction(int pid, const std::string& processName,
                                                                std::set<std::string>& declaredVars,
                                                                int currentNestLevel, int maxLines);

    static std::shared_ptr<Instruction> createForLoop(int pid, const std::string& processName, int maxLines,
                                                      std::set<std::string>& declaredVars, int currentNestLevel);
};
