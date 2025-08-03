#include "ConsoleManager.h"

#include <print>

#include "InstructionFactory.h"
#include "MainScreen.h"
#include "PagingAllocator.h"
#include "Process.h"
#include "ProcessScheduler.h"
#include "ProcessScreen.h"

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