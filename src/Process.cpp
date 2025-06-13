//
// Created by paren on 22/05/2025.
//

#include "Process.h"

#include <utility>
#include <chrono>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

/**
 * @brief Constructs a new Process object with the given ID and name.
 *        Initializes currentLine to 0 and totalLines to 50.
 *        Also generates a timestamp at the moment of creation.
 * @param id The unique identifier of the process.
 * @param name The name of the process.
 */
Process::Process(const int id, std::string  name)
    : processID(id), processName(std::move(name)), currentLine(0), totalLines(50) {
    timestamp = generateTimestamp();
}

/**
 * @brief Gets the unique identifier of the process.
 * @return Process ID.
 */
int Process::getID() const {
    return processID;
}

/**
 * @brief Gets the name of the process.
 * @return A reference to the process name.
 */
std::string Process::getName() const {
    return processName;
}

/**
 * @brief Retrieves the log entries associated with the process.
 * @return A reference to a vector of log strings.
 */
std::vector<std::string> Process::getLogs() const {
    return logs;
}

/**
 * @brief Retrieves the current line the process is executing.
 * @return Current line number.
 */
int Process::getCurrentLine() const {
    return currentLine;
}

/**
 * @brief Retrieves the total number of lines the process is expected to execute.
 * @return Total line count.
 */
int Process::getTotalLines() const {
    return totalLines;
}

/**
 * @brief Gets the timestamp of when the process was created.
 * @return A reference to the timestamp string.
 */
std::string& Process::getTimestamp() {
    return timestamp;
}

/**
 * @brief Adds a new log entry to the process's log.
 * @param entry The log string to add.
 */
void Process::log(const std::string& entry) {
    logs.push_back(entry);
}

/**
 * @brief Increments the current line number, up to the total number of lines.
 */
void Process::incrementLine() {
    if (currentLine < totalLines) {
        currentLine++;
    }

    // If done with instructions, trigger log writing
    if (currentLine >= totalLines) {
        writeLogToFile();
    }
}

void Process::writeLogToFile() const {
    try {
        // Ensure logs directory exists
        std::filesystem::create_directories("logs");

        // Prepare output file stream
        std::string filename = "logs/" + processName + ".txt";
        std::ofstream outFile(filename);

        if (!outFile.is_open()) {
            std::cerr << "Error: Could not open log file for process " << processName << "\n";
            return;
        }

        // Simulated CPU Core ID using thread ID modulus (for simulation purposes)
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

        // Write log entries to file
        for (const std::string& log : logs) {
            outFile << "(" << timestamp << ") Core: " << core_id << " \"" << log << "\"\n";
        }

        outFile.close();
    } catch (const std::exception& e) {
        std::cerr << "Exception during log writing: " << e.what() << '\n';
    }
}


/**
 * @brief Generates a formatted timestamp string representing the current local time.
 * @return A string formatted as MM/DD/YYYY, HH:MM:SS AM/PM.
 */
std::string Process::generateTimestamp() const {
    auto now = std::chrono::system_clock::now();
    std::time_t timeT = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&timeT);

    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%m/%d/%Y, %I:%M:%S %p");
    return oss.str();
}
