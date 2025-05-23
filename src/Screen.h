#pragma once

#include <string>
#include <vector>

/// @class Screen
/// @brief Serves as the interface of main menu and process screens.
///
/// Shared properties of main menu and processes which handles rendering and user input handling.
class Screen {
public:
    /// @brief Virtual Screen deconstructor
    virtual ~Screen() = default;

    /// @brief Render the screen
    virtual void render() = 0;

    /// @brief Handle user input on the screen
    virtual void handleUserInput() = 0;

    /// @brief Get the name of the screen
    virtual std::string getName() const = 0;

protected:
    /// @brief Helper function for splitting input
    virtual std::vector<std::string> splitInput(const std::string& input) = 0;
};
