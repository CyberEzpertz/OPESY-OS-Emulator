#include "InstructionFactory.h"

#include <cstdlib>
#include <format>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "ArithmeticInstruction.h"
#include "Config.h"
#include "DeclareInstruction.h"
#include "ForInstruction.h"
#include "PrintInstruction.h"
#include "SleepInstruction.h"

std::mt19937 InstructionFactory::rng(std::random_device{}());

constexpr int MAX_NESTED_LEVELS = 3;

std::string getRandomQuote() {
    static const std::vector<std::string> quotes = {
        "Hello, World!",
        "Code is poetry.",
        "Keep it simple.",
        "Think twice, code once.",
        "Fix the cause, not the symptom.",
        "0 is false. 1 is true. 2 is off by one.",
        "Segfault incoming!",
        "There is no cloud.",
        "rm -rf / — Oops.",
    };

    const int idx = InstructionFactory::generateRandomNum(0, quotes.size() - 1);
    return quotes[idx];
}

std::string getNewVarName(
    const std::vector<std::set<std::string>>& declaredVarsStack) {
    // Count total variables across all scopes to ensure uniqueness
    size_t totalVars = 0;
    std::set<std::string> allVars;

    for (const auto& scope : declaredVarsStack) {
        for (const auto& var : scope) {
            allVars.insert(var);
        }
    }

    std::string name;
    do {
        name = std::format("var_{}", totalVars);
        totalVars++;
    } while (allVars.find(name) != allVars.end());

    return name;
}

std::string getExistingVarName(
    const std::vector<std::set<std::string>>& declaredVarsStack) {
    if (declaredVarsStack.empty())
        return getNewVarName(declaredVarsStack);

    // Collect all variables from all scopes
    std::vector<std::string> allVars;
    for (const auto& scope : declaredVarsStack) {
        for (const auto& var : scope) {
            allVars.push_back(var);
        }
    }

    if (allVars.empty())
        return getNewVarName(declaredVarsStack);

    const int randomNum =
        InstructionFactory::generateRandomNum(0, allVars.size() - 1);
    return allVars[randomNum];
}

uint16_t getRandomUint16() {
    return static_cast<uint16_t>(
        InstructionFactory::generateRandomNum(0, UINT16_MAX));
}

uint8_t getRandomSleepTime() {
    return static_cast<uint8_t>(
        InstructionFactory::generateRandomNum(1, UINT8_MAX));
}

std::vector<std::shared_ptr<Instruction>>
InstructionFactory::generateInstructions(const int pid,
                                         const std::string process_name) {
    const int minLines = Config::getInstance().getMinInstructions();
    const int maxLines = Config::getInstance().getMaxInstructions();
    const int randMaxLines = generateRandomNum(minLines, maxLines);

    std::vector<std::shared_ptr<Instruction>> instructions;

    int accumulatedLines = 0;
    auto declaredVarsStack = std::vector<std::set<std::string>>{};

    // Initialize with global scope
    declaredVarsStack.push_back({});

    while (accumulatedLines < randMaxLines) {
        const int remainingLines = randMaxLines - accumulatedLines;

        auto instr = createRandomInstruction(
            pid, process_name, declaredVarsStack, 0, remainingLines);
        const int lines = instr->getLineCount();

        if (lines > remainingLines) {
            continue;
        }

        accumulatedLines += lines;
        instructions.push_back(instr);
    }

    return instructions;
}

int InstructionFactory::generateRandomNum(const int min, const int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

std::string getRandomVarName(
    const std::vector<std::set<std::string>>& declaredVarsStack) {
    // Check if any variables exist across all scopes
    bool hasVariables = false;
    for (const auto& scope : declaredVarsStack) {
        if (!scope.empty()) {
            hasVariables = true;
            break;
        }
    }

    const bool useExisting =
        hasVariables && InstructionFactory::generateRandomNum(0, 1) == 0;

    if (useExisting) {
        return getExistingVarName(declaredVarsStack);
    } else {
        return getNewVarName(declaredVarsStack);
    }
}

Operand getRandomOperand(
    const std::vector<std::set<std::string>>& declaredVarsStack) {
    if (InstructionFactory::generateRandomNum(0, 1) == 0) {
        return getRandomVarName(declaredVarsStack);
    } else {
        return static_cast<uint16_t>(
            InstructionFactory::generateRandomNum(0, UINT16_MAX));
    }
}

std::shared_ptr<Instruction> InstructionFactory::createRandomInstruction(
    const int pid, const std::string process_name,
    std::vector<std::set<std::string>>& declaredVarsStack,
    const int currentNestLevel, const int maxLines) {
    const std::string msg = std::format("Hello world from {}.", process_name);
    const bool isLoopable =
        currentNestLevel < MAX_NESTED_LEVELS && maxLines > 1;

    switch (generateRandomNum(0, isLoopable ? 5 : 4)) {
        case 0: {  // PRINT VARIABLE VALUE
            // Check if any variables exist across all scopes
            bool hasVariables = false;
            for (const auto& scope : declaredVarsStack) {
                if (!scope.empty()) {
                    hasVariables = true;
                    break;
                }
            }

            if (!hasVariables)
                return std::make_shared<PrintInstruction>(msg, pid);

            std::string var = getExistingVarName(declaredVarsStack);
            std::string message = std::format("The value of {} is: ", var);
            return std::make_shared<PrintInstruction>(message, pid, var);
        }
        case 1: {  // DECLARE
            std::string var = getNewVarName(declaredVarsStack);
            uint16_t val =
                static_cast<uint16_t>(generateRandomNum(0, UINT16_MAX));

            // Add to current (innermost) scope
            if (!declaredVarsStack.empty()) {
                declaredVarsStack.back().insert(var);
            }

            return std::make_shared<DeclareInstruction>(var, val, pid);
        }
        case 2: {  // SLEEP
            return std::make_shared<SleepInstruction>(getRandomSleepTime(),
                                                      pid);
        }
        case 3: {  // ADD
            std::string result = getRandomVarName(declaredVarsStack);
            Operand lhs = getRandomOperand(declaredVarsStack);
            Operand rhs = getRandomOperand(declaredVarsStack);

            // Add to current (innermost) scope
            if (!declaredVarsStack.empty()) {
                declaredVarsStack.back().insert(result);
            }

            return std::make_shared<ArithmeticInstruction>(result, lhs, rhs,
                                                           Operation::ADD, pid);
        }
        case 4: {  // SUBTRACT
            std::string result = getRandomVarName(declaredVarsStack);
            Operand lhs = getRandomOperand(declaredVarsStack);
            Operand rhs = getRandomOperand(declaredVarsStack);

            // Add to current (innermost) scope
            if (!declaredVarsStack.empty()) {
                declaredVarsStack.back().insert(result);
            }

            return std::make_shared<ArithmeticInstruction>(
                result, lhs, rhs, Operation::SUBTRACT, pid);
        }
        case 5: {
            return createForLoop(pid, process_name, maxLines, declaredVarsStack,
                                 currentNestLevel + 1);
        }
        default:;
    }

    // fallback
    return std::make_shared<PrintInstruction>("Fallback Instruction", pid);
}

std::shared_ptr<Instruction> InstructionFactory::createForLoop(
    const int pid, std::string process_name, const int maxLines,
    std::vector<std::set<std::string>>& declaredVarsStack,
    const int currentNestLevel) {
    if (maxLines <= 1 || currentNestLevel > MAX_NESTED_LEVELS) {
        // Not enough space or exceeded nest level; fallback
        return std::make_shared<PrintInstruction>("Invalid FOR loop", pid);
    }

    std::vector<std::shared_ptr<Instruction>> loopBody;

    // Max loop count is capped to 5, maxLines is there to ensure no overflow
    // Max lines will be minimum 2 because of if-statement above
    const int maxLoopCount = std::min(5, maxLines);

    // Loop must run at least 2 times
    int loopCount = (generateRandomNum(2, maxLoopCount));

    // Here we ensure safe number of body lines
    const int maxBodyLines = maxLines / loopCount;

    // Generate a random amount of max lines for this loop body specifically
    const int maxGeneratedLines = generateRandomNum(1, maxBodyLines);

    int accumulatedLines = 0;

    // Push new scope for loop body
    declaredVarsStack.push_back({});

    while (accumulatedLines < maxGeneratedLines) {
        const int remainingLines = maxGeneratedLines - accumulatedLines;

        const auto instr =
            createRandomInstruction(pid, process_name, declaredVarsStack,
                                    currentNestLevel + 1, remainingLines);

        const int lineCount = instr->getLineCount();

        // In the case that the max generated lines is violated, do not add the
        // current instruction. Just retry.
        if ((accumulatedLines + lineCount) > maxGeneratedLines) {
            continue;
        }

        accumulatedLines += lineCount;
        loopBody.push_back(instr);
    }

    // Pop the loop body scope
    declaredVarsStack.pop_back();

    return std::make_shared<ForInstruction>(pid, loopCount, loopBody);
}

std::vector<std::shared_ptr<Instruction>>
InstructionFactory::createAlternatingPrintAdd(const int pid) {
    const int minLines = Config::getInstance().getMinInstructions();
    const int maxLines = Config::getInstance().getMaxInstructions();
    const int randMaxLines = generateRandomNum(minLines, maxLines);

    std::vector<std::shared_ptr<Instruction>> instructions;

    for (int i = 0; i < randMaxLines; i++) {
        if (i % 2 == 0) {
            auto instr = std::make_shared<PrintInstruction>(
                PrintInstruction("Value from: ", pid, "x"));
            instructions.push_back(instr);
        } else {
            uint16_t randNum = generateRandomNum(1, 10);
            auto instr = std::make_shared<ArithmeticInstruction>(
                "x", "x", randNum, Operation::ADD, pid);
            instructions.push_back(instr);
        }
    }

    return instructions;
}