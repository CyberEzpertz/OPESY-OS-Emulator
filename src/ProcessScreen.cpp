#include "ProcessScreen.h"
#include "ConsoleManager.h"
//#include "Process.h"
#include <iostream>
#include <print>

ProcessScreen::ProcessScreen(std::shared_ptr<Process> process)
    : processPtr(std::move(process)) {}

void ProcessScreen::render() {

    std::println("\033[35m=== Process Information ===\033[0m");
    std::println("ID: {}", processPtr->getID());
    std::println("Name: {}", processPtr->getName());
    std::println("Timestamp: {}", processPtr->getTimestamp());
    std::println("Instruction Line: {}/{}",
               processPtr->getCurrentLine(),
               processPtr->getTotalLines());
    std::println("\n\033[32m--- Logs ---\033[0m");
    for (const auto& log : processPtr->getLogs()) {
        std::println(" * {}", log);
    }

    std::println("\033[36mType 'exit' to return to main menu or 'process-smi' to refresh.\033[0m");
}

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

