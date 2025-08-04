#pragma once

#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "Process.h"
#include "Screen.h"

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

    /// @brief Initializes the program with the provided configs
    /// and instantiates the MainScreen.
    void initialize();
    void initMainScreen();

    /// @brief Deleted copy assignment operator to enforce singleton pattern.
    void operator=(ConsoleManager const&) = delete;

    /// @brief Retrieves the singleton instance of ConsoleManager.
    /// @return Pointer to the ConsoleManager instance.
    static ConsoleManager& getInstance();

    /// @brief Switches the active screen to the given screen name.
    /// @param processName The name of the screen to switch to.
    void switchConsole(const std::string& processName);

    /// @brief Adds a new process to the manager.
    /// @param processName the name of the process to be created.
    /// @return True if the creation was successful, false otherwise.
    bool createProcess(const std::string& processName);
    std::shared_ptr<Process> createDummyProcess(const std::string& processName);

    /// @brief Returns whether the program is marked for exit.
    /// @return True if the program should exit, false otherwise.
    bool getHasExited() const;
    void createDummies(int count);

    /// @brief Clears the console screen using platform-specific commands.
    static void clearConsole();

    /// @brief Gets user input for the currently active screen.
    void getUserInput() const;

    /// @brief Signals the program to exit.
    void exitProgram();

    bool getHasInitialized() const;

    std::unordered_map<std::string, std::shared_ptr<Process>> getProcessNameMap();

    std::shared_ptr<Process> getProcessByPID(int processID);
    std::vector<std::shared_ptr<Process>> getProcessIdList();

    void returnToMainScreen();

    /// @return true if process creation was successful, false otherwise
    bool createProcessWithCustomInstructions(const std::string& processName,
                                            int memSize,
                                            const std::string& instrStr);

private:
    /// @brief Flag to indicate if the program should exit.
    bool hasExited = false;

    /// @brief Flag to indicate if the program has been initialized.
    bool hasInitialized = false;

    /// @brief Private constructor to enforce singleton pattern.
    ConsoleManager() = default;

    /// @brief Renders the currently active screen.
    ///
    /// @note This function should call the `render` method of the current
    /// screen once the screen interface is defined.
    void renderConsole() const;

    /// @brief The currently active screen.
    std::shared_ptr<Screen> currentScreen;

    /// @brief Map of available processes identified by name.
    std::unordered_map<std::string, std::shared_ptr<Process>> processNameMap;

    /// @brief List of processes with the ID as the key.
    std::vector<std::shared_ptr<Process>> processIDList;

    std::shared_mutex processListMutex;

    bool isPowerOfTwo(int n) const;

    bool validateMemorySize(int memSize) const;

    std::vector<std::string> parseInstructions(const std::string& instrStr) const;

    bool validateInstructionsFitMemory(const std::vector<std::string>& instructions, int memSize) const;
};
