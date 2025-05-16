#include <iostream>
#include <print>
#include <string>

void setColor(int color) {
    // ANSI escape codes for colors
    std::cout << "\033[" << color << "m";
}

void resetColor() {
    std::cout << "\033[0m";
}

void clrScreen() {
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\033[2J\033[1;1H";
#endif
}

void printHeader() {
    std::string asciiArt = "  ____ ____   ___  ____  _____ ______   __\n"
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

void printPlaceholder(std::string command) {
    std::println("'{}' command recognized. Doing something.", command);
}

int main() {
    printHeader();
    do {
        std::string input;
        std::print("Enter a command: ");
        std::getline(std::cin, input);

        if (input == "exit") {
            break;
        }

        if (input == "clear") {
            clrScreen();
            printHeader();
        } else if (input == "screen") {
            printPlaceholder(input);
        } else if (input == "scheduler-test") {
            printPlaceholder(input);
        } else if (input == "scheduler-stop") {
            printPlaceholder(input);
        } else if (input == "report-util") {
            printPlaceholder(input);
        } else if (input == "initialize") {
            printPlaceholder(input);
        } else {
            std::cout << "Unknown command: " << input << std::endl;
        }
    } while (true);

    return 0;
}