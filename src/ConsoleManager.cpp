#include "ConsoleManager.h"

#include <print>

/// Initializes a main menu screen when the singleton is created.
/// @todo Once Main Menu Screen is done, uncomment this and implement it
/// properly.
ConsoleManager::ConsoleManager() {
    // // Create the main menu screen
    // auto mainMenu = std::make_shared<MainMenuScreen>();
    //
    // // Add it to available screens by its name
    // availableScreens[mainMenu->getName()] = mainMenu;
    //
    // // Set it as the current screen
    // currScreen = mainMenu;
    // renderCurrScreen();
}

/// Returns a pointer to the lazily-initialized singleton instance.
///
/// Uses a function-local static to ensure thread-safe initialization on first
/// call.
ConsoleManager* ConsoleManager::getInstance() {
    static ConsoleManager instance;
    return &instance;
}

/// Switches the active screen if the given screen name exists.
///
/// Logs an error message if the screen name is not found.
void ConsoleManager::switchScreen(const std::string& screenName) {
    if (availableScreens.contains(screenName)) {
        currScreen = availableScreens[screenName];
    } else {
        std::print("No process named {} was found.", screenName);
    }
}

/// Registers a screen by name for future switching.
void ConsoleManager::addScreen(const std::string& screenName,
                               const std::shared_ptr<std::any>& screenPtr) {
    availableScreens[screenName] = screenPtr;
}

/// Clears the terminal output using platform-specific commands.
///
/// On Windows, uses `cls`. On Unix-like systems, uses ANSI escape codes.
void ConsoleManager::clearConsole() {
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\033[2J\033[1;1H";
#endif
}

/// Removes a screen by name from the list of available screens.
///
/// Does nothing if the screen name is not found.
void ConsoleManager::removeScreen(const std::string& screenName) {
    availableScreens.erase(screenName);
}

/// Renders the currently active screen.
///
/// @todo Call the screen’s actual render function once the Screen interface is
/// defined.
void ConsoleManager::renderCurrScreen() {
    // TODO: Call the current screen's render method here
}

/// Passes user input to the currently active screen for handling.
///
/// @todo Call the screen’s handleInput function once the Screen interface is
/// defined.
void ConsoleManager::getUserInput() {
    // TODO: Call the current screen's handleInput method here
}

/// Sets the exit flag to true, signaling the main loop to terminate.
void ConsoleManager::exitProgram() {
    hasExited = true;
}

/// Returns whether the application has been marked for exit.
bool ConsoleManager::getHasExited() const {
    return hasExited;
}
