#pragma once

#include <string>
#include <vector>

#include "Screen.h"

/// @class MainScreen
/// @brief A singleton object which serves as the main menu for the console.
///
/// The MainScreen class implements the Screen interface and acts as the entry
/// point for console-based user interaction. It provides rendering logic,
/// handles user input, and delegates commands to appropriate subsystems.
///
/// This class follows the singleton pattern to ensure only one instance of the
/// main menu exists.
class MainScreen final : public Screen {
public:
    /// @brief Deleted assignment operator to enforce singleton pattern.
    MainScreen& operator=(const MainScreen&) = delete;

    /// @brief Provides access to the singleton instance of MainScreen.
    /// @return Reference to the single instance of MainScreen.
    static MainScreen& getInstance();

    /// @brief Renders the main menu content to the console.
    void render() override;

    /// @brief Handles user input, parses commands, and performs corresponding
    /// actions.
    void handleUserInput() override;
    void handleScreenCommand(const std::vector<std::string>& tokens);

private:
    /// @brief Private constructor to enforce singleton pattern.
    MainScreen() = default;

    /// @brief Sets the console text color.
    /// @param color The ANSI color code to apply.
    static void setColor(int color);

    /// @brief Resets the console text color to default.
    static void resetColor();

    /// @brief Displays a placeholder message for unimplemented commands.
    /// @param command The command entered by the user.
    static void printPlaceholder(const std::string& command);
    void printProcessReport();
    void generateUtilizationReport();
    void generateProcessSMI();
    void generateVmStat();
};