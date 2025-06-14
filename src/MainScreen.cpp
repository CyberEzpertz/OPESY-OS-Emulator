#include "MainScreen.h"

#include <algorithm>
#include <iostream>
#include <print>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

#include "ConsoleManager.h"
#include "ProcessScheduler.h"

/// @brief Returns the singleton instance of MainScreen.
/// @return A single shared instance of MainScreen.
MainScreen& MainScreen::getInstance() {
    static MainScreen instance;
    return instance;
}

/// @brief Renders the main screen header and starts handling user input.
void MainScreen::render() {
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
    ConsoleManager& console = ConsoleManager::getInstance();

    if (cmd == "exit") {
        console.exitProgram();  // Trigger outer loop exit
    } else if (cmd == "clear") {
        console.clearConsole();
        render();
    } else if (cmd == "screen") {
        if (tokens.size() < 3) {
            if (tokens[1] == "-ls") {
                printProcessReport();
            } else {
                std::println("Not enough arguments for screen command.");
            }
        } else if (tokens[1] == "-s") {
            const bool success = console.createProcess(tokens[2]);
            if (success)
                console.switchConsole(tokens[2]);
        } else if (tokens[1] == "-r") {
            console.switchConsole(tokens[2]);
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

/// @brief Prints a placeholder message for recognized commands not yet
/// implemented.
/// @param command The command to acknowledge.
void MainScreen::printPlaceholder(const std::string& command) {
    std::println("'{}' command recognized. Doing something.", command);
}

void MainScreen::printProcessReport() {
    ProcessScheduler& scheduler = ProcessScheduler::getInstance();

    int availableCores = scheduler.getNumAvailableCores();
    int numCores = scheduler.getNumTotalCores();
    double cpuUtil =
        static_cast<double>(numCores - availableCores) / numCores * 100.0;

    auto processes = ConsoleManager::getInstance().getProcesses();

    std::vector<std::shared_ptr<Process>> sorted;

    for (const auto& proc : processes | std::views::values) {
        sorted.push_back(proc);
    }

    std::ranges::sort(sorted, [](const auto& a, const auto& b) {
        return a->getName() < b->getName();
    });

    std::println("CPU Utilization: {:.0f}%", cpuUtil);
    std::println("Cores used: {}", numCores - availableCores);
    std::println("Cores available: {}", availableCores);
    std::println("Total Cores: {}", numCores);

    std::println("{:->30}", "");
    std::println("Running processes:");

    for (const auto& process : sorted) {
        if (process->getStatus() != DONE) {
            std::string coreStr =
                (process->getCurrentCore() == -1)
                    ? "N/A"
                    : std::to_string(process->getCurrentCore());

            std::println("{:<10}\t({:<8})\tCore:\t{:<4}\t{} / {}",
                         process->getName(), process->getTimestamp(), coreStr,
                         process->getCurrentLine(), process->getTotalLines());
        }
    }

    std::println("\nFinished processes:");

    for (const auto& process : sorted) {
        if (process->getStatus() == DONE) {
            std::println("{:<10}\t({:<8})\tFinished\t{} / {}",
                         process->getName(), process->getTimestamp(),
                         process->getCurrentLine(), process->getTotalLines());
        }
    }

    std::println("{:->30}", "");
}
