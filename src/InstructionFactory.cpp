#include "InstructionFactory.h"

#include <cstdlib>
#include <format>
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

std::string getNewVarName(const std::set<std::string>& declaredVars) {
    std::string name = std::format("var_{}", declaredVars.size());
    return name;
}

std::string getExistingVarName(const std::set<std::string>& declaredVars) {
    if (declaredVars.empty())
        return getNewVarName(declaredVars);

    const int randomNum =
        InstructionFactory::generateRandomNum(0, declaredVars.size() - 1);
    auto it = declaredVars.begin();
    std::advance(it, randomNum);

    return *it;
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
InstructionFactory::generateInstructions(const int pid) {
    const int minLines = Config::getInstance().getMinInstructions();
    const int maxLines = Config::getInstance().getMaxInstructions();
    const int randMaxLines = generateRandomNum(minLines, maxLines);

    std::vector<std::shared_ptr<Instruction>> instructions;

    int accumulatedLines = 0;
    auto declaredVars = std::set<std::string>{};

    while (accumulatedLines < randMaxLines) {
        const int remainingLines = randMaxLines - accumulatedLines;

        auto instr =
            createRandomInstruction(pid, declaredVars, 0, remainingLines);
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

std::string getRandomVarName(const std::set<std::string>& declaredVars) {
    const bool useExisting = !declaredVars.empty() &&
                             InstructionFactory::generateRandomNum(0, 1) == 0;

    if (useExisting) {
        return getExistingVarName(declaredVars);
    } else {
        return getNewVarName(declaredVars);
    }
}

Operand getRandomOperand(const std::set<std::string>& declaredVars) {
    if (InstructionFactory::generateRandomNum(0, 1) == 0) {
        return getRandomVarName(declaredVars);
    } else {
        return static_cast<uint16_t>(
            InstructionFactory::generateRandomNum(0, UINT16_MAX));
    }
}

std::shared_ptr<Instruction> InstructionFactory::createRandomInstruction(
    const int pid, std::set<std::string>& declaredVars,
    const int currentNestLevel, const int maxLines) {
    const std::string msg = std::format('Hello world from {}.', pid);
    const bool isLoopable =
        currentNestLevel < MAX_NESTED_LEVELS && maxLines > 1;

    switch (generateRandomNum(0, isLoopable ? 6 : 5)) {
        case 0: {  // PRINT RANDOM QUOTE
            return std::make_shared<PrintInstruction>(msg, pid);
        }
        case 1: {  // PRINT VARIABLE VALUE
            if (declaredVars.empty())
                return std::make_shared<PrintInstruction>(msg, pid);

            std::string var = getExistingVarName(declaredVars);
            std::string message = std::format("The value of {} is: ", var);
            return std::make_shared<PrintInstruction>(message, pid, var);
        }
        case 2: {  // DECLARE
            std::string var = getNewVarName(declaredVars);
            uint16_t val =
                static_cast<uint16_t>(generateRandomNum(0, UINT16_MAX));
            declaredVars.insert(var);
            return std::make_shared<DeclareInstruction>(var, val, pid);
        }
        case 3: {  // SLEEP
            return std::make_shared<SleepInstruction>(getRandomSleepTime(),
                                                      pid);
        }
        case 4: {  // ADD
            std::string result = getRandomVarName(declaredVars);
            Operand lhs = getRandomOperand(declaredVars);
            Operand rhs = getRandomOperand(declaredVars);
            declaredVars.insert(result);
            return std::make_shared<ArithmeticInstruction>(result, lhs, rhs,
                                                           Operation::ADD, pid);
        }
        case 5: {  // SUBTRACT
            std::string result = getRandomVarName(declaredVars);
            Operand lhs = getRandomOperand(declaredVars);
            Operand rhs = getRandomOperand(declaredVars);
            declaredVars.insert(result);
            return std::make_shared<ArithmeticInstruction>(
                result, lhs, rhs, Operation::SUBTRACT, pid);
        }
        case 6: {
            return createForLoop(pid, maxLines, declaredVars,
                                 currentNestLevel + 1);
        }
        default:;
    }

    // fallback
    return std::make_shared<PrintInstruction>("Fallback Instruction", pid);
}

std::shared_ptr<Instruction> InstructionFactory::createForLoop(
    const int pid, const int maxLines, std::set<std::string>& declaredVars,
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

    while (accumulatedLines < maxGeneratedLines) {
        const int remainingLines = maxGeneratedLines - accumulatedLines;

        const auto instr = createRandomInstruction(
            pid, declaredVars, currentNestLevel + 1, remainingLines);

        const int lineCount = instr->getLineCount();

        // In the case that the max generated lines is violated, do not add the
        // current instruction. Just retry.
        if ((accumulatedLines + lineCount) > maxGeneratedLines) {
            continue;
        }

        accumulatedLines += lineCount;
        loopBody.push_back(instr);
    }

    return std::make_shared<ForInstruction>(pid, loopCount, loopBody);
}
