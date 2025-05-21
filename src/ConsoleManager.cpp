#include "ConsoleManager.h"

#include <print>

ConsoleManager* ConsoleManager::instancePtr = nullptr;

void ConsoleManager::init() {
    if (instancePtr == nullptr)
        instancePtr = new ConsoleManager();
}

ConsoleManager* ConsoleManager::getInstance() {
    return instancePtr;
}

void ConsoleManager::switchScreen(const std::string& screenName) {
    if (availableScreens.contains(screenName)) {
        currScreen = availableScreens[screenName];
    }
}

void ConsoleManager::addScreen(const std::string& screenName,
                               const std::shared_ptr<std::any>& screenPtr) {
    availableScreens[screenName] = screenPtr;
}

void ConsoleManager::clearConsole() {
#ifdef _WIN32
    system("cls");
#else
    std::cout << "\033[2J\033[1;1H";
#endif
}

void ConsoleManager::removeScreen(const std::string& screenName) {
    availableScreens.erase(screenName);
}

void ConsoleManager::renderCurrScreen() {
    // TODO: Call the current screen's render method here
}

void ConsoleManager::getUserInput() {
    // TODO: Call the current screen's handleInput method here
}

void ConsoleManager::exitProgram() {
    hasExited = true;
}

bool ConsoleManager::getHasExited() const {
    return hasExited;
}
