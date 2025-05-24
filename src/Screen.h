#pragma once

#include <sstream>
#include <string>
#include <vector>

/// @class Screen
/// @brief Serves as the interface of main menu and process screens.
///
/// Shared properties of main menu and processes which handles rendering and
/// user input handling.
class Screen {
public:
    /// @brief Virtual Screen deconstructor
    virtual ~Screen() = default;

    /// @brief Render the screen
    virtual void render() = 0;

    /// @brief Handle user input on the screen
    virtual void handleUserInput() = 0;

protected:
    /// @brief Splits a user input string into a list of space-separated tokens.
    /// @param input The full input line.
    /// @return A vector containing individual command tokens.
    virtual std::vector<std::string> splitInput(const std::string& input) {
        std::istringstream iss(input);
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    };
};
