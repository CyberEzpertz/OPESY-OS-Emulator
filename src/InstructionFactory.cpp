#include "InstructionFactory.h"

#include <cstdlib>
#include <format>
#include <iomanip>
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
#include "ReadInstruction.h"
#include "SleepInstruction.h"
#include "WriteInstruction.h"

std::mt19937 InstructionFactory::rng(std::random_device{}());

constexpr int MAX_NESTED_LEVELS = 3;
constexpr int MAX_VARIABLES = 32;
constexpr int INSTRUCTION_SIZE = 2;
constexpr int SYMBOL_TABLE_SIZE = 64;

std::string getNewVarName(const std::set<std::string>& declaredVars) {
    size_t totalVars = declaredVars.size();
    std::string name;

    do {
        name = std::format("var_{}", totalVars++);
    } while (declaredVars.contains(name));

    return name;
}

std::string getExistingVarName(const std::set<std::string>& declaredVars) {
    if (declaredVars.empty())
        return getNewVarName(declaredVars);

    std::vector<std::string> allVars(declaredVars.begin(), declaredVars.end());
    int randomIndex = InstructionFactory::generateRandomNum(0, allVars.size() - 1);
    return allVars[randomIndex];
}

uint16_t getRandomUint16() {
    return static_cast<uint16_t>(InstructionFactory::generateRandomNum(0, UINT16_MAX));
}

uint8_t getRandomSleepTime() {
    return static_cast<uint8_t>(InstructionFactory::generateRandomNum(1, UINT8_MAX));
}

std::vector<std::shared_ptr<Instruction>> InstructionFactory::generateInstructions(const int pid,
                                                                                   const std::string& processName,
                                                                                   const int requiredMemory) {
    const int minLines = Config::getInstance().getMinInstructions();
    const int maxLines = Config::getInstance().getMaxInstructions();
    const int randMaxLines = generateRandomNum(minLines, maxLines);

    std::vector<std::shared_ptr<Instruction>> instructions;
    int accumulatedLines = 0;

    std::set<std::string> declaredVars;
    const int startMemory = randMaxLines * INSTRUCTION_SIZE + SYMBOL_TABLE_SIZE;

    // Leave a chance of error for the READ/WRITEs
    constexpr double errorChance = 0.01;
    const int errorMemory = requiredMemory * errorChance;
    const int endMemory = startMemory + requiredMemory - SYMBOL_TABLE_SIZE + errorMemory;

    while (accumulatedLines < randMaxLines) {
        const int remainingLines = randMaxLines - accumulatedLines;
        auto instr = createRandomInstruction(pid, processName, declaredVars, 0, remainingLines, startMemory, endMemory);
        const int lines = instr->getLineCount();

        if (lines > remainingLines)
            continue;

        if (lines > 1) {
            auto forInstr = std::dynamic_pointer_cast<ForInstruction>(instr);

            if (forInstr) {
                auto expanded = forInstr->expand();
                instructions.insert(instructions.end(), expanded.begin(), expanded.end());
            } else {
                // fallback, just in case it's not a for loop for some reason
                instructions.push_back(instr);
            }
        } else {
            instructions.push_back(instr);
        }

        accumulatedLines += lines;
    }

    return instructions;
}

int InstructionFactory::generateRandomNum(const int min, const int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

std::string getRandomVarName(const std::set<std::string>& declaredVars) {
    const bool useExisting = !declaredVars.empty() && InstructionFactory::generateRandomNum(0, 1) == 0;

    return useExisting ? getExistingVarName(declaredVars) : getNewVarName(declaredVars);
}

Operand getRandomOperand(const std::set<std::string>& declaredVars) {
    if (InstructionFactory::generateRandomNum(0, 1) == 0) {
        return getRandomVarName(declaredVars);
    }

    return static_cast<uint16_t>(InstructionFactory::generateRandomNum(0, UINT16_MAX));
}

std::shared_ptr<Instruction> InstructionFactory::createRandomInstruction(const int pid, const std::string& processName,
                                                                         std::set<std::string>& declaredVars,
                                                                         const int currentNestLevel, const int maxLines,
                                                                         const int startMemory, const int endMemory) {
    const std::string msg = std::format("Hello world from {}.", processName);

    const bool isLoopable = currentNestLevel < MAX_NESTED_LEVELS && maxLines > 1;

    switch (generateRandomNum(0, isLoopable ? 7 : 6)) {
        case 0: {  // PRINT VARIABLE VALUE
            // Check if any variables exist across all scopes
            bool hasVariables = false;
            for (const auto& scope : declaredVars) {
                if (!scope.empty()) {
                    hasVariables = true;
                    break;
                }
            }

            if (!hasVariables)
                return std::make_shared<PrintInstruction>(msg, pid);

            std::string var = getExistingVarName(declaredVars);
            std::string message = std::format("The value of {} is: ", var);
            return std::make_shared<PrintInstruction>(message, pid, var);
        }
        case 1: {  // DECLARE
            std::string var = getNewVarName(declaredVars);
            auto val = static_cast<uint16_t>(generateRandomNum(0, UINT16_MAX));

            // Only add it if it's below the max variables limit
            if (declaredVars.size() < MAX_VARIABLES) {
                declaredVars.insert(var);
            }

            return std::make_shared<DeclareInstruction>(var, val, pid);
        }
        case 2: {  // SLEEP
            return std::make_shared<SleepInstruction>(getRandomSleepTime(), pid);
        }
        case 3: {  // ADD
            std::string result = getRandomVarName(declaredVars);
            Operand lhs = getRandomOperand(declaredVars);
            Operand rhs = getRandomOperand(declaredVars);

            // Only add it if it's below the max variables limit
            if (declaredVars.size() < MAX_VARIABLES) {
                declaredVars.insert(result);
            }

            return std::make_shared<ArithmeticInstruction>(result, lhs, rhs, Operation::ADD, pid);
        }
        case 4: {  // SUBTRACT
            std::string result = getRandomVarName(declaredVars);
            Operand lhs = getRandomOperand(declaredVars);
            Operand rhs = getRandomOperand(declaredVars);

            // Only add it if it's below the max variables limit
            if (declaredVars.size() < MAX_VARIABLES) {
                declaredVars.insert(result);
            }

            return std::make_shared<ArithmeticInstruction>(result, lhs, rhs, Operation::SUBTRACT, pid);
        }
        case 5: {  // WRITE(address, value)
            int address = generateRandomNum(startMemory, endMemory - 2);
            uint16_t value = getRandomUint16();
            return std::make_shared<WriteInstruction>(address, value, pid);
        }
        case 6: {  // READ(var, address)
            std::string result = getRandomVarName(declaredVars);
            int address = generateRandomNum(startMemory, endMemory - 2);
            return std::make_shared<ReadInstruction>(result, address, pid);
        }
        case 7: {
            return createForLoop(pid, processName, maxLines, declaredVars, currentNestLevel + 1, startMemory,
                                 endMemory);
        }
        default:;
    }

    // fallback
    return std::make_shared<PrintInstruction>("Fallback Instruction", pid);
}

std::shared_ptr<Instruction> InstructionFactory::createForLoop(const int pid, const std::string& processName,
                                                               const int maxLines, std::set<std::string>& declaredVars,
                                                               const int currentNestLevel, const int startMemory,
                                                               const int endMemory) {
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

        const auto instr = createRandomInstruction(pid, processName, declaredVars, currentNestLevel + 1, remainingLines,
                                                   startMemory, endMemory);

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

std::vector<std::shared_ptr<Instruction>> InstructionFactory::createAlternatingPrintAdd(const int pid) {
    const int minLines = Config::getInstance().getMinInstructions();
    const int maxLines = Config::getInstance().getMaxInstructions();
    const int randMaxLines = generateRandomNum(minLines, maxLines);

    std::vector<std::shared_ptr<Instruction>> instructions;

    for (int i = 0; i < randMaxLines; i++) {
        if (i % 2 == 0) {
            auto instr = std::make_shared<PrintInstruction>(PrintInstruction("Value from: ", pid, "x"));
            instructions.push_back(instr);
        } else {
            uint16_t randNum = generateRandomNum(1, 10);
            auto instr = std::make_shared<ArithmeticInstruction>("x", "x", randNum, Operation::ADD, pid);
            instructions.push_back(instr);
        }
    }

    return instructions;
}

std::shared_ptr<Instruction> InstructionFactory::deserializeInstruction(std::istream& is) {
    std::string type;
    is >> type;

    if (type == "PRINT") {
        int pid;
        bool hasVar;
        std::optional<std::string> varName = std::nullopt;
        std::string message;

        is >> pid >> hasVar;

        if (hasVar) {
            std::string var;
            is >> var;
            varName = var;
        }

        is >> std::quoted(message);

        if (hasVar) {
            return std::make_shared<PrintInstruction>(message, pid, varName.value());
        }

        return std::make_shared<PrintInstruction>(message, pid);
    }
    if (type == "DECLARE") {
        std::string var;
        uint16_t value;
        int pid;
        is >> var >> value >> pid;
        return std::make_shared<DeclareInstruction>(var, value, pid);
    }
    if (type == "SLEEP") {
        int duration, pid;
        is >> duration >> pid;
        return std::make_shared<SleepInstruction>(duration, pid);
    }

    if (type == "ARITH") {
        std::string resultName, lhsStr, rhsStr;
        int opInt, pid;
        is >> resultName >> lhsStr >> rhsStr >> opInt >> pid;

        auto parseOperand = [](const std::string& token) -> Operand {
            try {
                // Try to parse as uint16_t
                return static_cast<uint16_t>(std::stoul(token));
            } catch (...) {
                // Otherwise treat as string (e.g. variable name)
                return token;
            }
        };

        Operand lhs = parseOperand(lhsStr);
        Operand rhs = parseOperand(rhsStr);
        auto op = static_cast<Operation>(opInt);

        return std::make_shared<ArithmeticInstruction>(resultName, lhs, rhs, op, pid);
    }

    if (type == "WRITE") {
        int addr, pid;
        uint16_t val;
        is >> addr >> val >> pid;
        return std::make_shared<WriteInstruction>(addr, val, pid);
    }
    if (type == "READ") {
        std::string var;
        int addr, pid;
        is >> var >> addr >> pid;
        return std::make_shared<ReadInstruction>(var, addr, pid);
    }

    if (type == "FOR") {
        int pid, totalLoops;
        size_t bodySize;
        is >> pid >> totalLoops >> bodySize;
        is.ignore();  // skip newline after the header line

        std::vector<std::shared_ptr<Instruction>> body;
        while (body.size() < bodySize) {
            std::string line;
            std::getline(is, line);

            // Skip empty lines or lines that are just whitespace
            if (line.empty() || std::all_of(line.begin(), line.end(), isspace)) {
                continue;
            }

            // Use a new stringstream for each line to deserialize
            std::istringstream lineStream(line);
            std::shared_ptr<Instruction> instr = deserializeInstruction(lineStream);
            body.push_back(instr);
        }

        std::string endLine;
        std::getline(is, endLine);
        if (endLine != "END") {
            throw std::runtime_error("Expected END after FOR loop body, got: " + endLine);
        }

        return std::make_shared<ForInstruction>(pid, totalLoops, body);
    }

    throw std::runtime_error("Unknown instruction type: " + type);
}
