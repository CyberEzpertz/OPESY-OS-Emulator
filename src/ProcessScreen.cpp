#include "ProcessScreen.h"

#include "ConsoleManager.h"
// #include "Process.h"
#include <iostream>
#include <print>

/// @brief Constructs a ProcessScreen with a specific process instance.
///
/// Initializes the internal processPtr with the given shared pointer to a
/// Process. This screen will display and interact with the provided process.
///
/// @param process A shared pointer to the process to display and control.
ProcessScreen::ProcessScreen(std::shared_ptr<Process> process)
    : processPtr(std::move(process)) {
}

/// @brief Renders detailed information about the current process.
///
/// Displays the process's ID, name, timestamp, current instruction progress,
/// and a list of logged messages. Also displays navigation instructions
/// for the user.
/// @brief Renders detailed information about the current process.
///
/// Displays the process's ID, name, timestamp, current instruction progress,
/// and a list of logged messages. Also displays navigation instructions
/// for the user.
void ProcessScreen::render() {
    std::println(
        "\n\033[35;1m========== Process Information ==========\033[0m");

    std::println("\033[1m{:>20}:\033[0m {}", "ID", processPtr->getID());
    std::println("\033[1m{:>20}:\033[0m {}", "Name", processPtr->getName());
    std::println("\033[1m{:>20}:\033[0m {}", "Timestamp",
                 processPtr->getTimestamp());
    std::println("\033[1m{:>20}:\033[0m {}/{}", "Instruction Line",
                 processPtr->getCurrentLine(), processPtr->getTotalLines());

    std::println("\n\033[32;1m------------- Logs -------------\033[0m");

    const auto& logs = processPtr->getLogs();
    if (logs.empty()) {
        std::println(" (No logs available)");
    } else {
        for (const auto& log : logs) {
            std::println("{}", log);
        }
    }

    if (processPtr->getStatus() == DONE)
        std::println("\n\033[1mProcess Finished!\033[0m");

    std::println("\n\033[36m[Type '\033[1mexit\033[0m\033[36m' to return to "
                 "the main menu or '\033[1mprocess-smi\033[0m\033[36m' to "
                 "refresh process information]\033[0m");
}

/// @brief Handles user input specific to the ProcessScreen context.
///
/// Recognized commands:
/// - "process-smi": Refreshes the screen by re-rendering the current process
/// info.
/// - "exit": Returns to the main screen.
/// - Any other command results in an error message.
void ProcessScreen::handleUserInput() {
    std::string input;
    std::print("[Process] Enter command: ");
    std::getline(std::cin, input);

    if (input == "process-smi") {
        render();
    } else if (input == "exit") {
        ConsoleManager::getInstance().returnToMainScreen();
    } else {
        std::println("Unknown command: '{}'", input);
    }
}
