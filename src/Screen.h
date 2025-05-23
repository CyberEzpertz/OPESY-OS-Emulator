#pragma once

#include <string>
#include <vector>

class Screen {
public:
    virtual ~Screen() = default;

    // Render the screen
    virtual void render() = 0;

    // Handle user input on the screen
    virtual void handleUserInput() = 0;

    // Get the name of the screen
    virtual std::string getName() const = 0;

protected:
    // Helper function for splitting input (can be moved to a utility class if needed)
    virtual std::vector<std::string> splitInput(const std::string& input) = 0;
};
