#include "ConsoleManager.h"

#include <print>

#include "MainScreen.h"
#include "Process.h"

/// Initializes a main menu screen when the singleton is created.
/// @todo Once Main Menu Screen is done, uncomment this and implement it
/// properly.
ConsoleManager::ConsoleManager() {
    // Create the main menu screen
    const auto mainMenu =
        std::make_shared<MainScreen>(MainScreen::getInstance());

    // Add it to available screens by its name
    // availableScreens[mainMenu->getName()] = mainMenu;

    // Set it as the current screen
    currentScreen = mainMenu;
    renderConsole();
}

/// Returns a pointer to the lazily-initialized singleton instance.
///
/// Uses a function-local static to ensure thread-safe initialization on first
/// call.
const ConsoleManager& ConsoleManager::getInstance() {
    static ConsoleManager instance;
    return instance;
}

/// Switches the active console if the given process name exists.
///
/// Logs an error message if the screen name is not found.
void ConsoleManager::switchConsole(const std::string& processName) {
    if (processes.contains(processName)) {
        currentScreen = processes[processName];
    } else {
        std::print("No process named {} was found.", processName);
    }
}

/// Registers a process using its name for future switching.
void ConsoleManager::addProcess(const std::shared_ptr<std::any>& processPtr) {
    // TODO: Uncomment this once process has been implemented.
    // std::string processName = processPtr->getName();
    // processes[processName] = processPtr;
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

/// Renders the currently active screen.
///
/// @todo Call the screen’s actual render function once the Screen interface is
/// defined.
void ConsoleManager::renderConsole() {
    // TODO: Call the current screen's render method here
    currentScreen->render();
}

/// Passes user input to the currently active screen for handling.
///
/// @todo Call the screen’s handleInput function once the Screen interface is
/// defined.
void ConsoleManager::getUserInput() {
    // TODO: Call the current screen's handleInput method here
    currentScreen->handleUserInput();
}

/// Sets the exit flag to true, signaling the main loop to terminate.
void ConsoleManager::exitProgram() {
    hasExited = true;
}

/// Returns whether the application has been marked for exit.
bool ConsoleManager::getHasExited() const {
    return hasExited;
}
