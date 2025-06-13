#pragma once

#include <string>
#include <vector>
#include <ctime>

/**
 * @class Process
 * @brief Represents a simulated process in the CLI system.
 *
 * The Process class is responsible for maintaining its ID, name, instructions,
 * logs, and timestamp of creation. It also handles progression through instructions
 * and logging of print commands to a file.
 */
class Process {
private:
    int processID;                           ///< Unique ID of the process.
    std::string processName;                 ///< Name of the process.
    std::vector<std::string> logs;           ///< List of log messages (simulated PRINT commands).
    int currentLine = 0;                     ///< Current instruction line.
    int totalLines = 50;                     ///< Total instruction lines (hardcoded to 50).
    std::time_t creationTimestamp;           ///< Timestamp of when this process was created.

public:
    /**
     * @brief Constructs a Process instance with the given name and ID.
     * @param name Name of the process.
     * @param id Unique ID of the process.
     */
    Process(const std::string& name, int id);

    /**
     * @brief Increments the current instruction line.
     *        When the process finishes all lines, it writes logs to file.
     */
    void incrementLine();

    /**
     * @brief Adds a log entry to the process.
     * @param logMessage The message to be logged.
     */
    void addLog(const std::string& logMessage);

    /**
     * @brief Writes all logs (simulated PRINT instructions) to a file with timestamp and core ID.
     *
     * File will be written as: ./logs/<processName>.txt
     * Format: (MM/DD/YYYY, HH:MM:SS AM/PM) Core: X "message here"
     */
    void writeLogToFile() const;

    /// @return Unique process ID.
    int getID() const;

    /// @return Name of the process.
    std::string getName() const;

    /// @return Current instruction line.
    int getCurrentLine() const;

    /// @return Total number of instruction lines.
    int getTotalLines() const;

    /// @return Timestamp of process creation.
    std::time_t getCreationTimestamp() const;
};
