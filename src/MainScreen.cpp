#include "MainScreen.h"

#include <iostream>
#include <print>
#include <sstream>
#include <string>
#include <vector>

MainScreen MainScreen::getInstance() {
    static MainScreen instance;
    return instance;
}

void MainScreen::render() {
    printHeader();
    handleUserInput();
}

void MainScreen::handleUserInput() {
    std::string input;
    while (true) {
        std::print("Enter a command: ");
        std::getline(std::cin, input);

        if (input == "exit") {
            break;
        }

        if (input == "clear") {
            clrScreen();
            printHeader();
        } else {
            std::vector<std::string> tokens = splitInput(input);
            if (!tokens.empty()) {
                const std::string& cmd = tokens[0];

                if (cmd == "screen" && tokens.size() >= 3 && tokens[1] == "-s") {
                    printPlaceholder("Create screen: " + tokens[2]);
                    // TODO: Create & store new process, switch to process screen
                } else if (cmd == "screen" && tokens.size() >= 3 && tokens[1] == "-r") {
                    printPlaceholder("Resume screen: " + tokens[2]);
                    // TODO: Resume screen if exists
                } else if (cmd == "scheduler-test" || cmd == "scheduler-stop" ||
                           cmd == "report-util" || cmd == "initialize") {
                    printPlaceholder(cmd);
                } else {
                    std::cout << "Unknown command: " << input << std::endl;
                }
            }
        }
    }
}

std::string MainScreen::getName() const {
    return "MainMenu";
}

std::vector<std::string> MainScreen::splitInput(const std::string& input) {
    std::istringstream iss(input);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

void MainScreen::setColor(int color) {
    std::cout << "\033[" << color << "m";
}

void MainScreen::resetColor() {
    std::cout << "\033[0m";
}

void MainScreen::clrScreen() {
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\033[2J\033[1;1H";
#endif
}

void MainScreen::printHeader() {
    const std::string asciiArt =
        "  ____ ____   ___  ____  _____ ______   __\n"
        " / ___/ ___| / _ \\|  _ \\| ____/ ___\\ \\ / /\n"
        "| |   \\___ \\| | | | |_) |  _| \\___ \\\\ V / \n"
        "| |___ ___) | |_| |  __/| |___ ___) || |  \n"
        " \\____|____/ \\___/|_|   |_____|____/ |_|  \n";

    std::cout << asciiArt << std::endl;

    setColor(36);
    std::println("Hello, Welcome to the CSOPESY commandline!");
    resetColor();

    setColor(33);
    std::println("Type 'exit' to quit, 'clear' to clear the screen");
    resetColor();
}

void MainScreen::printPlaceholder(const std::string& command) {
    std::println("'{}' command recognized. Doing something.", command);
}
