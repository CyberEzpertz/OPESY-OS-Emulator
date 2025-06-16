#include "ConsoleManager.h"

int main() {
    ConsoleManager& console = ConsoleManager::getInstance();
    console.initMainScreen();

    while (!console.getHasExited()) {
        console.getUserInput();
    }

    return 0;
}