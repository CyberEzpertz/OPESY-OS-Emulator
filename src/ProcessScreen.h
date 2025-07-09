#pragma once

#include <memory>

#include "Process.h"
#include "Screen.h"

/// @class ProcessScreen
/// @brief A screen that displays and interacts with a specific process.
///
/// The ProcessScreen class implements the Screen interface and is responsible
/// for rendering process-related information to the console, such as progress,
/// instructions, logs, and metadata. It also handles user input to allow basic
/// interaction with the current process.
///
/// Each instance of ProcessScreen is associated with a specific Process object.
class ProcessScreen : public Screen {
public:
    /// @brief Constructs a ProcessScreen with the given process.
    /// @param process A shared pointer to the Process to be displayed.
    explicit ProcessScreen(std::shared_ptr<Process> process);

    /// @brief Renders the process information to the console.
    void render() override;

    /// @brief Handles user input while on the process screen.
    void handleUserInput() override;

private:
    /// @brief The process currently being displayed and interacted with.
    std::shared_ptr<Process> processPtr;
};
