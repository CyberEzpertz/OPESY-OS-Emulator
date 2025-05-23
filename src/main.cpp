#include <iostream>
#include <print>
#include <string>

#include "ConsoleManager.h"
#include "MainScreen.h"


int main() {
    auto* console = ConsoleManager::getInstance();

    while (!console->getHasExited()) {
        console->getUserInput();
    }

    return 0;
}