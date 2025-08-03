#include "ConsoleManager.h"

#include <print>
#include <sstream>
#include <algorithm>
#include <cmath>

#include "InstructionFactory.h"
#include "MainScreen.h"
#include "PagingAllocator.h"
#include "Process.h"
#include "ProcessScheduler.h"
#include "ProcessScreen.h"

// Constants for memory validation
constexpr int MIN_MEMORY_SIZE = 64;
constexpr int MAX_MEMORY_SIZE = 65536;
constexpr int INSTRUCTION_SIZE = 2;  // bytes
constexpr int SYMBOL_TABLE_SIZE = 64;  // bytes
constexpr int MIN_INSTRUCTIONS = 1;
constexpr int MAX_INSTRUCTIONS = 50;

/// Initializes the console manager with the main screen and renders it.
void ConsoleManager::initialize() {
    hasInitialized = true;
    Config::getInstance().loadFromFile();
    PagingAllocator::getInstance();
    ProcessScheduler::getInstance().initialize();
    ProcessScheduler::getInstance().start();

    // NOTE: This is for debugging only.
    // Config::getInstance().print();
}

void ConsoleManager::initMainScreen() {
    currentScreen = std::make_shared<MainScreen>(MainScreen::getInstance());
    renderConsole();
}

/// Returns a pointer to the lazily-initialized singleton instance.
///
/// Uses a function-local static to ensure thread-safe initialization on first
/// call.
ConsoleManager& ConsoleManager::getInstance() {
    static ConsoleManager instance;
    return instance;
}

/// Switches the active console if the given process name exists.
///
/// Logs an error message if the screen name is not found.
void ConsoleManager::switchConsole(const std::string& processName) {
    if (processNameMap.contains(processName)) {
        if (processNameMap[processName]->getStatus() == ProcessStatus::DONE) {
            std::println("Process {} not found.", processName);
        } else {
            currentScreen = std::make_shared<ProcessScreen>(processNameMap[processName]);
            clearConsole();
            currentScreen->render();
        }
    } else {
        std::println("Error: No process named {} was found.", processName);
    }
}

/// Helper function to check if a number is a power of 2
bool ConsoleManager::isPowerOfTwo(int n) const {
    return n > 0 && (n & (n - 1)) == 0;
}

/// Validates memory size according to requirements
bool ConsoleManager::validateMemorySize(int memSize) const {
    if (memSize < MIN_MEMORY_SIZE || memSize > MAX_MEMORY_SIZE) {
        return false;
    }
    return isPowerOfTwo(memSize);
}

/// Parses instruction string separated by semicolons
std::vector<std::string> ConsoleManager::parseInstructions(const std::string& instrStr) const {
    std::vector<std::string> instructions;
    std::stringstream ss(instrStr);
    std::string instruction;

    while (std::getline(ss, instruction, ';')) {
        // Trim whitespace
        instruction.erase(instruction.begin(),
            std::find_if(instruction.begin(), instruction.end(),
                [](unsigned char ch) { return !std::isspace(ch); }));
        instruction.erase(
            std::find_if(instruction.rbegin(), instruction.rend(),
                [](unsigned char ch) { return !std::isspace(ch); }).base(),
            instruction.end());

        if (!instruction.empty()) {
            instructions.push_back(instruction);
        }
    }

    return instructions;
}

/// Validates if instructions fit within memory constraints
bool ConsoleManager::validateInstructionsFitMemory(const std::vector<std::string>& instructions, int memSize) const {
    if (instructions.size() < MIN_INSTRUCTIONS || instructions.size() > MAX_INSTRUCTIONS) {
        return false;
    }

    int requiredMemory = (instructions.size() * INSTRUCTION_SIZE) + SYMBOL_TABLE_SIZE;
    return requiredMemory <= memSize;
}

/// Creates a process with custom instructions and memory size
bool ConsoleManager::createProcessWithCustomInstructions(const std::string& processName,
                                                        int memSize,
                                                        const std::string& instrStr) {
    // Check if process name already exists
    if (processNameMap.contains(processName)) {
        std::println("Error: Process '{}' already exists.", processName);
        return false;
    }

    // Validate memory size
    if (!validateMemorySize(memSize)) {
        std::println("Invalid memory allocation. Value must be a power of 2 between 64 and 65536.");
        return false;
    }

    // Parse instructions
    std::vector<std::string> instructionStrings = parseInstructions(instrStr);

    if (instructionStrings.empty()) {
        std::println("Error: No valid instructions provided.");
        return false;
    }

    // Validate instruction count and memory fit
    if (!validateInstructionsFitMemory(instructionStrings, memSize)) {
        if (instructionStrings.size() < MIN_INSTRUCTIONS || instructionStrings.size() > MAX_INSTRUCTIONS) {
            std::println("Error: Number of instructions must be between {} and {}.",
                        MIN_INSTRUCTIONS, MAX_INSTRUCTIONS);
        } else {
            int requiredMemory = (instructionStrings.size() * INSTRUCTION_SIZE) + SYMBOL_TABLE_SIZE;
            std::println("Error: Instructions require {} bytes but only {} bytes available.",
                        requiredMemory, memSize);
        }
        return false;
    }

    // Create the process
    std::unique_lock lock(processListMutex);
    const int PID = processIDList.size();

    // Create process with the specified memory size
    // Note: We don't add instruction size here since setInstructions will handle it
    const auto newProcess = std::make_shared<Process>(PID, processName, memSize);
    processNameMap[processName] = newProcess;
    processIDList.push_back(newProcess);

    // Convert instruction strings to actual instruction objects
    // This assumes InstructionFactory has a method to create instructions from strings
    std::vector<std::shared_ptr<Instruction>> instructions;
    try {
        instructions = InstructionFactory::createInstructionsFromStrings(instructionStrings, PID);
    } catch (const std::exception& e) {
        // If instruction creation fails, remove the process and return false
        processNameMap.erase(processName);
        processIDList.pop_back();
        std::println("Error: Failed to create instructions: {}", e.what());
        return false;
    }

    // Set instructions with addToMemory=false since we already allocated the exact memory size
    // The user-specified memSize already accounts for instructions + symbol table + data
    newProcess->setInstructions(instructions, false);
    ProcessScheduler::getInstance().scheduleProcess(newProcess);

    std::println("Process '{}' created successfully with {} instructions and {} bytes of memory.",
                processName, instructions.size(), memSize);

    return true;
}

/// Creates and registers a process using its name for future switching.
/// Returns true if creation was successful, false if not.
bool ConsoleManager::createProcess(const std::string& processName) {
    // Don't allow duplicate process names because we use that to access them
    if (processNameMap.contains(processName)) {
        std::println("Error: Process '{}' already exists.", processName);
        return false;
    }

    std::unique_lock lock(processListMutex);
    const int PID = processIDList.size();

    const auto newProcess = std::make_shared<Process>(PID, processName);
    processNameMap[processName] = newProcess;
    processIDList.push_back(newProcess);

    const auto instructions = InstructionFactory::createAlternatingPrintAdd(PID);
    newProcess->setInstructions(instructions);
    ProcessScheduler::getInstance().scheduleProcess(newProcess);

    return true;
}

bool ConsoleManager::createDummyProcess(const std::string& processName) {
    // Don't allow duplicate process names because we use that to access them
    if (processNameMap.contains(processName)) {
        std::println("Error: Process '{}' already exists.", processName);
        return false;
    }

    std::unique_lock lock(processListMutex);
    const int PID = processNameMap.size();

    const auto minMem = Config::getInstance().getMinMemPerProc();
    const auto maxMem = Config::getInstance().getMaxMemPerProc();
    const auto requiredMemory = InstructionFactory::generateRandomNum(minMem, maxMem);

    const auto newProcess = std::make_shared<Process>(PID, processName, requiredMemory);
    processNameMap[processName] = newProcess;
    processIDList.push_back(newProcess);

    const std::vector<std::shared_ptr<Instruction>> instructions =
        InstructionFactory::generateInstructions(PID, processName, requiredMemory);

    newProcess->setInstructions(instructions, true);
    ProcessScheduler::getInstance().scheduleProcess(newProcess);

    return true;
}

/// Clears the terminal output using platform-specific commands.
///
/// On Windows, uses `cls`. On Unix-like systems, uses ANSI escape codes.
void ConsoleManager::clearConsole() {
#ifdef _WIN32
    system("cls");
#else
    std::print("\033[2J\033[1;1H");
#endif
}

void ConsoleManager::returnToMainScreen() {
    currentScreen = std::make_shared<MainScreen>(MainScreen::getInstance());
    clearConsole();
    renderConsole();
}

/// Renders the currently active screen.
void ConsoleManager::renderConsole() const {
    currentScreen->render();
}

/// Passes user input to the currently active screen for handling.
void ConsoleManager::getUserInput() const {
    currentScreen->handleUserInput();
}

/// Sets the exit flag to true, signaling the main loop to terminate.
void ConsoleManager::exitProgram() {
    hasExited = true;
}

bool ConsoleManager::getHasInitialized() const {
    return hasInitialized;
}

std::unordered_map<std::string, std::shared_ptr<Process>> ConsoleManager::getProcessNameMap() {
    std::shared_lock lock(processListMutex);
    return processNameMap;
}

std::shared_ptr<Process> ConsoleManager::getProcessByPID(const int processID) {
    std::shared_lock lock(processListMutex);

    if (processID >= processIDList.size()) {
        std::println("Tried to access {} but size is {}", processID, processIDList.size());
        return nullptr;
    }

    return processIDList[processID];
}

std::vector<std::shared_ptr<Process>> ConsoleManager::getProcessIdList() {
    std::shared_lock lock(processListMutex);

    return processIDList;
}

/// Returns whether the application has been marked for exit.
bool ConsoleManager::getHasExited() const {
    return hasExited;
}

void ConsoleManager::createDummies(const int count) {
    for (int i = 1; i <= count; ++i) {
        std::string processName = std::format("process_{:02}", i);
        createDummyProcess(processName);
    }
}