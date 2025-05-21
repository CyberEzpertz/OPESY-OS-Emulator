#pragma once
#include <any>
#include <memory>
#include <string>
#include <unordered_map>

class ConsoleManager {
public:
    // Singleton pattern
    static void init();
    ConsoleManager(ConsoleManager&) = delete;
    void operator=(ConsoleManager const&) = delete;
    static ConsoleManager* getInstance();

    void switchScreen(std::string screenName);
    void addScreen(std::string screenName, std::shared_ptr<std::any> screenPtr);
    void removeScreen(std::string screenName);

    bool getHasExited();
    void exitProgram();

private:
    static ConsoleManager* instancePtr;
    bool hasExited = false;

    void clearScreen();
    void renderCurrScreen();
    void getUserInput();

    // Please change the anys into a Screen class pointer once it's created
    std::shared_ptr<std::any> currScreen;
    std::unordered_map<std::string, std::shared_ptr<std::any>> availableScreens;
};
