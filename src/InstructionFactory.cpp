#include "InstructionFactory.h"

#include <cstdlib>
#include <format>
#include <iomanip>
#include <memory>
#include <print>
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
    constexpr double errorChance = 0.1;
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
            bool hasVar = generateRandomNum(0, 1) % 2 == 1;
            int address = generateRandomNum(startMemory, endMemory - 2);

            if (hasVar) {
                uint16_t value = getRandomUint16();
                return std::make_shared<WriteInstruction>(address, value, pid);
            }

            auto varName = getRandomVarName(declaredVars);
            return std::make_shared<WriteInstruction>(address, varName, pid);
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
        bool hasVar;
        int addr, pid;
        is >> hasVar >> addr;

        if (hasVar) {
            std::string var;
            is >> var >> pid;
            return std::make_shared<WriteInstruction>(addr, var, pid);
        }

        uint16_t val;
        is >> val >> pid;
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

std::vector<std::shared_ptr<Instruction>> InstructionFactory::createInstructionsFromStrings(
    const std::vector<std::string>& instructionStrings, int processID) {
    std::vector<std::shared_ptr<Instruction>> instructions;
    instructions.reserve(instructionStrings.size());

    for (const std::string& instrStr : instructionStrings) {
        try {
            auto instruction = parseInstructionString(instrStr, processID);
            instructions.push_back(instruction);
        } catch (const std::exception& e) {
            // Re-throw with more context about which instruction failed
            throw std::runtime_error("Failed to parse instruction '" + instrStr + "': " + e.what());
        }
    }

    return instructions;
}

std::shared_ptr<Instruction> InstructionFactory::parseInstructionString(const std::string& instrStr, int processID) {
    std::istringstream iss(instrStr);

    std::string command;
    char ch;
    while (iss.get(ch)) {
        if (std::isalnum(ch) || ch == '_') {
            command += ch;
        } else {
            iss.unget();  // Put back non-word character
            break;
        }
    }

    auto parseHexAddress = [](const std::string& token) -> int {
        std::string hexPart = token;

        // Strip 0x or 0X prefix if present
        if (hexPart.starts_with("0x") || hexPart.starts_with("0X")) {
            hexPart = hexPart.substr(2);
        }

        int addr;
        auto [ptr, ec] = std::from_chars(hexPart.data(), hexPart.data() + hexPart.size(), addr, 16);
        if (ec != std::errc()) {
            throw std::runtime_error("Invalid hex address: " + token);
        }
        return addr;
    };

    // Convert to uppercase for case-insensitive comparison
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);

    if (command == "PRINT") {
        std::string remaining;
        std::getline(iss, remaining);

        // Trim leading whitespace
        remaining.erase(remaining.begin(), std::find_if(remaining.begin(), remaining.end(),
                                                        [](unsigned char ch) { return !std::isspace(ch); }));

        if (remaining.empty() || remaining.front() != '(' || remaining.back() != ')') {
            throw std::runtime_error("PRINT expression must be in the format: PRINT(\"text\" [+ var])");
        }

        std::string expr = remaining.substr(1, remaining.size() - 2);  // remove parentheses

        // Find optional '+' for concatenation
        auto plusPos = expr.find('+');
        if (plusPos == std::string::npos) {
            // No '+', treat entire thing as message OR variable
            expr.erase(expr.begin(),
                       std::find_if(expr.begin(), expr.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            expr.erase(
                std::find_if(expr.rbegin(), expr.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
                expr.end());

            if (expr.front() == '"' && expr.back() == '"') {
                std::string literal = expr.substr(1, expr.size() - 2);
                return std::make_shared<PrintInstruction>(literal, processID);  // Just a string literal
            } else {
                return std::make_shared<PrintInstruction>("", processID, expr);  // Just a variable
            }
        } else {
            // There is a '+' -> string + variable
            std::string left = expr.substr(0, plusPos);
            std::string right = expr.substr(plusPos + 1);

            // Trim
            left.erase(left.begin(),
                       std::find_if(left.begin(), left.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            left.erase(
                std::find_if(left.rbegin(), left.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
                left.end());

            right.erase(right.begin(),
                        std::find_if(right.begin(), right.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            right.erase(
                std::find_if(right.rbegin(), right.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(),
                right.end());

            if (!(left.front() == '"' && left.back() == '"')) {
                throw std::runtime_error("Left side of '+' in PRINT must be a quoted string");
            }

            std::string literal = left.substr(1, left.size() - 2);
            std::string varName = right;
            return std::make_shared<PrintInstruction>(literal, processID, varName);
        }
    }

    if (command == "DECLARE") {
        // Format: DECLARE variable value
        std::string variable;
        uint16_t value;
        iss >> variable >> value;

        if (variable.empty() || iss.fail()) {
            throw std::runtime_error("DECLARE instruction requires variable name and value");
        }

        return std::make_shared<DeclareInstruction>(variable, value, processID);
    }
    if (command == "SLEEP") {
        // Format: SLEEP duration
        int duration;
        iss >> duration;

        if (iss.fail()) {
            throw std::runtime_error("SLEEP instruction requires duration");
        }

        return std::make_shared<SleepInstruction>(duration, processID);
    }
    if (command == "ADD" || command == "SUB") {
        // Format: ADD result lhs rhs
        std::string resultName, lhsStr, rhsStr;
        iss >> resultName >> lhsStr >> rhsStr;

        if (resultName.empty() || lhsStr.empty() || rhsStr.empty()) {
            throw std::runtime_error(command + " instruction requires result, lhs, and rhs operands");
        }

        auto parseOperand = [](const std::string& token) -> Operand {
            try {
                // Try to parse as uint16_t
                return static_cast<uint16_t>(std::stoul(token));
            } catch (...) {
                // Otherwise treat as string (variable name)
                return token;
            }
        };

        Operand lhs = parseOperand(lhsStr);
        Operand rhs = parseOperand(rhsStr);

        Operation op;
        if (command == "ADD")
            op = Operation::ADD;
        else if (command == "SUB")
            op = Operation::SUBTRACT;

        return std::make_shared<ArithmeticInstruction>(resultName, lhs, rhs, op, processID);
    }
    if (command == "WRITE") {
        // Format: WRITE address value
        std::string addrToken, valueToken;
        iss >> addrToken >> valueToken;

        if (addrToken.empty() || valueToken.empty()) {
            throw std::runtime_error("WRITE instruction requires address and value");
        }

        // Parse address as hex
        int addr = parseHexAddress(addrToken);

        // Parse valueToken as either literal or variable
        uint16_t literalValue;
        auto [vptr, vec] = std::from_chars(valueToken.data(), valueToken.data() + valueToken.size(), literalValue);
        if (vec == std::errc()) {
            return std::make_shared<WriteInstruction>(addr, literalValue, processID);
        }

        return std::make_shared<WriteInstruction>(addr, valueToken, processID);
    }

    if (command == "READ") {
        // Format: READ variable address
        std::string variable, addrToken;
        iss >> variable >> addrToken;

        if (variable.empty() || addrToken.empty()) {
            throw std::runtime_error("READ instruction requires variable name and address");
        }

        int addr = parseHexAddress(addrToken);

        return std::make_shared<ReadInstruction>(variable, addr, processID);
    }
    if (command == "FOR") {
        // FOR loops are complex and typically not single-line
        // This is a simplified version - you might want to handle this differently
        throw std::runtime_error("FOR loops are not supported in single-line instruction format. Use separate "
                                 "instruction files for complex control structures.");
    }
    throw std::runtime_error("Unknown instruction: " + command);
}

// Note: You'll need to add this method declaration to your InstructionFactory.h:
// static std::shared_ptr<Instruction> parseInstructionString(const std::string& instrStr, int processID);

// Example usage patterns for the instruction parser:
//
// PRINT "Hello World"                    -> PrintInstruction("Hello World", pid)
// PRINT var "Value is: "                 -> PrintInstruction("Value is: ", pid, "var")
// DECLARE x 10                           -> DeclareInstruction("x", 10, pid)
// SLEEP 1000                             -> SleepInstruction(1000, pid)
// ADD result 5 10                        -> ArithmeticInstruction("result", 5, 10, ADD, pid)
// SUB result var1 var2                   -> ArithmeticInstruction("result", "var1", "var2", SUB, pid)
// MUL result var1 5                      -> ArithmeticInstruction("result", "var1", 5, MUL, pid)
// DIV result 20 var2                     -> ArithmeticInstruction("result", 20, "var2", DIV, pid)
// WRITE 100 255                          -> WriteInstruction(100, 255, pid)
// READ var 100                           -> ReadInstruction("var", 100, pid)