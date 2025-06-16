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
        R"(
__________                         .__       ________    _________
\______   \ _______  __ ___________|__| ____ \_____  \  /   _____/
 |       _// __ \  \/ // __ \_  __ \  |/ __ \ /   |   \ \_____  \
 |    |   \  ___/\   /\  ___/|  | \/  \  ___//    |    \/        \
 |____|_  /\___  >\_/  \___  >__|  |__|\___  >_______  /_______  /
        \/     \/          \/              \/        \/        \/
)";

    setColor(35);
    std::print("{}", asciiArt);
    resetColor();

    std::println("{:->60}", "");
    setColor(36);
    std::println("Hello, Welcome to the ReverieOS commandline!");
    resetColor();

    if (ConsoleManager::getInstance().getHasInitialized()) {
        setColor(33);
        std::println("Type 'exit' to quit, 'clear' to clear the screen");
        resetColor();
    } else {
        setColor(33);
        std::println("Type 'initialize' to start the program, exit' to quit");
        resetColor();
    }

    std::println("{:->60}", "");
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
/// - "screen -ls": Displays the processes
/// - "scheduler-test", "scheduler-stop", "report-util", "initialize":
/// Placeholders for other commands.
///
/// If the command is unrecognized, prints an error message.
///
void MainScreen::handleUserInput() {
    std::string input;
    setColor(35);
    std::print("reverie-✦> ");
    resetColor();

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

    if (!console.getHasInitialized()) {
        if (cmd == "exit") {
            console.exitProgram();
        } else if (cmd == "initialize") {
            console.initialize();
            std::println("Program initialized");
        } else {
            std::println("Error: Program has not been initialized. Please type "
                         "\"initialize\" before proceeding.");
        }
        return;
    }

    if (cmd == "exit") {
        console.exitProgram();  // Trigger outer loop exit
    } else if (cmd == "clear") {
        ConsoleManager::clearConsole();
        render();
    } else if (cmd == "screen") {
        handleScreenCommand(tokens);
    } else if (cmd == "initialize") {
        std::println("Program has already been initialized.");
    } else if (cmd == "scheduler-test" || cmd == "scheduler-stop" ||
               cmd == "report-util") {
        printPlaceholder(cmd);
    } else {
        std::println("Error: Unknown command {}", cmd);
    }
}

void MainScreen::handleScreenCommand(const std::vector<std::string>& tokens) {
    auto& console = ConsoleManager::getInstance();

    if (tokens.size() < 2) {
        std::println("Error: Not enough arguments for screen command.");
        return;
    }

    const std::string& flag = tokens[1];

    if (flag == "-ls") {
        if (tokens.size() > 2) {
            std::println("Error: Too many arguments for -ls.");
        } else {
            printProcessReport();
        }
        return;
    }

    if (flag == "-s" || flag == "-r") {
        if (tokens.size() < 3) {
            std::println("Error: Missing process name for {} flag.", flag);
            return;
        }

        const std::string& processName = tokens[2];

        if (flag == "-s") {
            if (console.createProcess(processName)) {
                console.switchConsole(processName);
            }
        } else {  // -r
            console.switchConsole(processName);
        }

        return;
    }

    // If the flag is not recognized
    std::println("Invalid screen flag: {}", flag);
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

    std::println("Waiting Processes");

    for (const auto& process : sorted) {
        if (process->getStatus() == WAITING) {
            std::string coreStr =
                (process->getCurrentCore() == -1)
                    ? "N/A"
                    : std::to_string(process->getCurrentCore());

            std::println("{:<10}\t({:<8})\tCore:\t{:<4}\t{} / {}",
                         process->getName(), process->getTimestamp(), coreStr,
                         process->getCurrentLine(), process->getTotalLines());
        }
    }

    std::println("Running processes:");

    for (const auto& process : sorted) {
        if (process->getStatus() != DONE && process->getStatus() != WAITING) {
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
