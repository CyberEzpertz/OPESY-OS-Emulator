#pragma once

#include <any>
#include <memory>
#include <string>
#include <unordered_map>

/// @class ConsoleManager
/// @brief Manages console screen rendering and input handling using a singleton
/// pattern.
///
/// This class is responsible for storing, switching between, and rendering
/// different console "screens" (currently represented by `std::any` but should
/// later be replaced with a proper `Screen` class). It also handles application
/// exit and console clearing.
class ConsoleManager {
public:
    /// @brief Deleted copy constructor to enforce singleton pattern.
    ConsoleManager(ConsoleManager&) = delete;

    /// @brief Deleted copy assignment operator to enforce singleton pattern.
    void operator=(ConsoleManager const&) = delete;

    /// @brief Retrieves the singleton instance of ConsoleManager.
    /// @return Pointer to the ConsoleManager instance.
    static ConsoleManager* getInstance();

    /// @brief Switches the active screen to the given screen name.
    /// @param screenName The name of the screen to switch to.
    void switchScreen(const std::string& screenName);

    /// @brief Adds a new screen to the manager.
    /// @param screenName The name identifier of the screen.
    /// @param screenPtr Shared pointer to the screen object.
    void addScreen(const std::string& screenName,
                   const std::shared_ptr<std::any>& screenPtr);

    /// @brief Removes a screen by its name.
    /// @param screenName The name identifier of the screen to remove.
    void removeScreen(const std::string& screenName);

    /// @brief Returns whether the program is marked for exit.
    /// @return True if the program should exit, false otherwise.
    bool getHasExited() const;

    /// @brief Gets user input for the currently active screen.
    ///
    /// @note This function should call the `handleInput` method of the current
    /// screen once the screen interface is defined.
    void getUserInput();

    /// @brief Signals the program to exit.
    void exitProgram();

private:
    /// @brief Flag to indicate if the program should exit.
    bool hasExited = false;

    /// @brief Private constructor to enforce singleton pattern.
    ConsoleManager();

    /// @brief Clears the console screen using platform-specific commands.
    static void clearConsole();

    /// @brief Renders the currently active screen.
    ///
    /// @note This function should call the `render` method of the current
    /// screen once the screen interface is defined.
    void renderCurrScreen();

    /// @brief The currently active screen.
    ///
    /// @note This should be changed to a pointer to a `Screen` class once it's
    /// created.
    std::shared_ptr<std::any> currScreen;

    /// @brief Map of available screens identified by name.
    ///
    /// @note Screen pointers are stored as `std::any` for now, but should be
    /// changed to the appropriate screen type once it's implemented.
    std::unordered_map<std::string, std::shared_ptr<std::any>> availableScreens;
};
