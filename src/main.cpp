#include <iostream>
#include <print>
#include <string>

#include "ConsoleManager.h"
#include "MainScreen.h"
#include "ProcessScheduler.h"

int main() {
    ConsoleManager& console = ConsoleManager::getInstance();
    ProcessScheduler& scheduler = ProcessScheduler::getInstance();

    scheduler.initialize(4);
    console.initialize();
    console.createDummies(10);
    scheduler.start();

    while (!console.getHasExited()) {
        console.getUserInput();
    }

    return 0;
}