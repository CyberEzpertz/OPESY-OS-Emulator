#include "ConsoleManager.h"

int main() {
    ConsoleManager& console = ConsoleManager::getInstance();

    while (!console.getHasExited()) {
        console.getUserInput();
    }

    return 0;
}