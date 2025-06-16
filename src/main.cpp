#include <print>

#include "Config.h"
#include "ConsoleManager.h"
#include "ProcessScheduler.h"

int main() {
    ConsoleManager& console = ConsoleManager::getInstance();
    ProcessScheduler& scheduler = ProcessScheduler::getInstance();

    scheduler.initialize(4);
    console.initialize();
    console.createDummies(10);
    scheduler.start();
    Config::getInstance().print();

    while (!console.getHasExited()) {
        console.getUserInput();
    }
    scheduler.stop();
    std::print("Test");

    return 0;
}