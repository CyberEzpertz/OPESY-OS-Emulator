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

    void switchScreen(const std::string& screenName);
    void addScreen(const std::string& screenName,
                   const std::shared_ptr<std::any>& screenPtr);
    void removeScreen(const std::string& screenName);

    bool getHasExited() const;
    void exitProgram();

private:
    static ConsoleManager* instancePtr;
    bool hasExited = false;
    ConsoleManager() = default;

    static void clearConsole();
    void renderCurrScreen();
    void getUserInput();

    // Please change the anys into a Screen class pointer once it's created
    std::shared_ptr<std::any> currScreen;
    std::unordered_map<std::string, std::shared_ptr<std::any>> availableScreens;
};
