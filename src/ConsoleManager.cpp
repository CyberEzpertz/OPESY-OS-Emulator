#include "ConsoleManager.h"

#include <print>

#include "MainScreen.h"
#include "Process.h"
#include "ProcessScheduler.h"
#include "ProcessScreen.h"

/// Initializes the console manager with the main screen and renders it.
/// TODO: Will read the config file and use the parameters there in the future.
void ConsoleManager::initialize() {
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
    if (processes.contains(processName)) {
        currentScreen = std::make_shared<ProcessScreen>(processes[processName]);
        clearConsole();
        currentScreen->render();
    } else {
        std::println("Error: No process named {} was found.", processName);
    }
}

/// Creates and registers a process using its name for future switching.
/// Returns true if creation was successful, false if not.
bool ConsoleManager::createProcess(const std::string& processName) {
    // Don't allow duplicate process names because we use that to access them
    if (processes.contains(processName)) {
        std::println("Error: Process '{}' already exists.", processName);
        return false;
    }

    const int PID = processes.size() + 1;

    auto newProcess = std::make_shared<Process>(PID, processName);
    processes[processName] = newProcess;

    ProcessScheduler::getInstance().scheduleProcess(newProcess);

    return true;
}

bool ConsoleManager::createDummyProcess(const std::string& processName) {
    // Don't allow duplicate process names because we use that to access them
    if (processes.contains(processName)) {
        std::println("Error: Process '{}' already exists.", processName);
        return false;
    }

    const int PID = processes.size() + 1;

    const auto newProcess = std::make_shared<Process>(PID, processName);
    std::vector<std::shared_ptr<Instruction>> instructions;

    for (int i = 0; i < 100; i++) {
        instructions.push_back(std::make_shared<PrintInstruction>(
            std::format("Hello world from {}!", newProcess->getName()),
            newProcess));
    }

    newProcess->setInstructions(instructions);

    processes[processName] = newProcess;

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
void ConsoleManager::getUserInput() {
    currentScreen->handleUserInput();
}

/// Sets the exit flag to true, signaling the main loop to terminate.
void ConsoleManager::exitProgram() {
    hasExited = true;
}
std::unordered_map<std::string, std::shared_ptr<Process>>
ConsoleManager::getProcesses() {
    return processes;
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