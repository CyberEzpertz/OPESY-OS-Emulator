#include <iostream>
#include <print>
#include <string>

#include "ConsoleManager.h"
#include "MainScreen.h"


int main() {
    auto* console = ConsoleManager::getInstance();
    auto* mainScreen = MainScreen::getInstance();
    mainScreen->render();

    while (!console->getHasExited()) {
        console->getUserInput();
    }

    return 0;
}