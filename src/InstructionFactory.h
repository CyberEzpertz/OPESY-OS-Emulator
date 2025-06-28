#pragma once

#include <memory>
#include <random>
#include <set>
#include <vector>

#include "Instruction.h"
#include "PrintInstruction.h"

class InstructionFactory {
public:
    static std::vector<std::shared_ptr<Instruction>> generateInstructions(
        int pid,
        std::string process_name);
    static std::mt19937 rng;
    static int generateRandomNum(int min, int max);

private:
    static std::shared_ptr<Instruction> createRandomInstruction(
        int pid, std::string process_name, std::vector<std::set<std::string>>& declaredVarsStack, int currentNestLevel,
        int maxLines);

    static std::shared_ptr<Instruction> createForLoop(
        int pid, std::string process_name, int maxLines, std::vector<std::set<std::string>>& declaredVarsStack,
        int currentNestLevel);
};
