#include "Process.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

/**
 * @brief Constructs a Process with a given name and ID, and captures creation timestamp.
 */
Process::Process(const std::string& name, int id)
    : processName(name), processID(id), creationTimestamp(std::time(nullptr)) {}

/**
 * @brief Increments the instruction pointer by 1.
 *        Calls writeLogToFile() once the process finishes all instructions.
 */
void Process::incrementLine() {
    if (currentLine < totalLines) {
        currentLine++;
    }

    if (currentLine >= totalLines) {
        writeLogToFile();
    }
}

/**
 * @brief Adds a log entry to the internal log list.
 * @param logMessage The log message to store.
 */
void Process::addLog(const std::string& logMessage) {
    logs.push_back(logMessage);
}

/**
 * @brief Writes all log messages to a file in ./logs folder with timestamp and CPU core info.
 *
 * Each log line is written in the following format:
 * (MM/DD/YYYY, HH:MM:SS AM/PM) Core: <core_id> "log message"
 */
void Process::writeLogToFile() const {
    try {
        // Create logs directory if it doesn't exist
        std::filesystem::create_directories("logs");

        std::string filename = "logs/" + processName + ".txt";
        std::ofstream outFile(filename);

        if (!outFile.is_open()) {
            std::cerr << "Error: Unable to open log file for process " << processName << "\n";
            return;
        }

        // Simulated CPU core ID
        auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
        int core_id = static_cast<int>(thread_id % std::thread::hardware_concurrency());

        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
        std::tm localTime;

#ifdef _WIN32
        localtime_s(&localTime, &timeNow);
#else
        localtime_r(&timeNow, &localTime);
#endif

        std::ostringstream timestampStream;
        timestampStream << std::put_time(&localTime, "%m/%d/%Y, %I:%M:%S %p");
        std::string timestamp = timestampStream.str();

        // Write each log line with timestamp and core info
        for (const std::string& log : logs) {
            outFile << "(" << timestamp << ") Core: " << core_id << " \"" << log << "\"\n";
        }

        outFile.close();
    } catch (const std::exception& e) {
        std::cerr << "Exception in writeLogToFile(): " << e.what() << '\n';
    }
}

// ---------------------- Getters ----------------------

/**
 * @return The process ID.
 */
int Process::getID() const {
    return processID;
}

/**
 * @return The name of the process.
 */
std::string Process::getName() const {
    return processName;
}

/**
 * @return The current line of instruction being executed.
 */
int Process::getCurrentLine() const {
    return currentLine;
}

/**
 * @return The total number of instructions.
 */
int Process::getTotalLines() const {
    return totalLines;
}

/**
 * @return The creation timestamp of the process.
 */
std::time_t Process::getCreationTimestamp() const {
    return creationTimestamp;
}
