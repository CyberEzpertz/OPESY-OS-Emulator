#include "MainScreen.h"

#include <iostream>
#include <print>
#include <sstream>
#include <string>
#include <vector>

#include "ConsoleManager.h"

/// @brief Returns the singleton instance of MainScreen.
/// @return A single shared instance of MainScreen.
MainScreen& MainScreen::getInstance() {
    static MainScreen instance;
    return instance;
}

/// @brief Renders the main screen header and starts handling user input.
void MainScreen::render() {
    printHeader();
    handleUserInput();
}
///
/// @brief Processes a single user command from standard input.
///
/// Reads a line of input, normalizes whitespace, and parses it into tokens.
/// Executes commands like 'exit', 'clear', 'screen' options, and predefined
/// commands.
///
/// Recognized commands:
/// - "exit": Signals the ConsoleManager to exit the program loop.
/// - "clear": Clears the console screen and prints the header.
/// - "screen -s <name>": Placeholder for creating a new screen.
/// - "screen -r <name>": Placeholder for resuming a screen.
/// - "scheduler-test", "scheduler-stop", "report-util", "initialize":
/// Placeholders for other commands.
///
/// If the command is unrecognized, prints an error message.
///
void MainScreen::handleUserInput() {
    std::string input;
    std::print("Enter a command: ");
    std::getline(std::cin, input);

    // Normalize whitespace (collapse multiple spaces)
    std::istringstream iss(input);
    std::string word;
    std::vector<std::string> tokens;
    while (iss >> word) {
        tokens.push_back(word);
    }

    if (tokens.empty())
        return;

    const std::string& cmd = tokens[0];

    if (cmd == "exit") {
        ConsoleManager::getInstance().exitProgram();  // Trigger outer loop exit
    } else if (cmd == "clear") {
        clrScreen();
        printHeader();
    } else if (cmd == "screen") {
        if (tokens.size() < 3) {
            std::println("Not enough arguments for screen command.");
        } else if (tokens[1] == "-s") {
            auto newProcess = std::make_shared<Process>(1, tokens[2]);
            ConsoleManager::getInstance().addProcess(newProcess);
            ConsoleManager::getInstance().switchConsole(tokens[2]);
        } else if (tokens[1] == "-r") {
            ConsoleManager::getInstance().switchConsole(tokens[2]);
        } else {
            std::println("Invalid screen flag: {}", tokens[1]);
        }
    } else if (cmd == "scheduler-test" || cmd == "scheduler-stop" ||
               cmd == "report-util" || cmd == "initialize") {
        printPlaceholder(cmd);
    } else {
        std::println("Unknown command: {}", cmd);
    }
}

/// @brief Sets the console text color using ANSI escape codes.
/// @param color The color code to apply.
void MainScreen::setColor(int color) {
    std::print("\033[{}m", color);
}

/// @brief Resets the console text color to the default.
void MainScreen::resetColor() {
    std::print("\033[0m");
}

/// @brief Clears the console screen using platform-specific commands.
void MainScreen::clrScreen() {
#ifdef _WIN32
    system("cls");
#else
    std::print("\033[2J\033[1;1H");
#endif
}

/// @brief Displays the ASCII header and instructions on the main screen.
void MainScreen::printHeader() {
    const std::string asciiArt =
        "  ____ ____   ___  ____  _____ ______   __\n"
        " / ___/ ___| / _ \\|  _ \\| ____/ ___\\ \\ / /\n"
        "| |   \\___ \\| | | | |_) |  _| \\___ \\\\ V / \n"
        "| |___ ___) | |_| |  __/| |___ ___) || |  \n"
        " \\____|____/ \\___/|_|   |_____|____/ |_|  \n";

    std::print("{}", asciiArt);

    setColor(36);
    std::println("Hello, Welcome to the CSOPESY commandline!");
    resetColor();

    setColor(33);
    std::println("Type 'exit' to quit, 'clear' to clear the screen");
    resetColor();
}

/// @brief Prints a placeholder message for recognized commands not yet
/// implemented.
/// @param command The command to acknowledge.
void MainScreen::printPlaceholder(const std::string& command) {
    std::println("'{}' command recognized. Doing something.", command);
}
