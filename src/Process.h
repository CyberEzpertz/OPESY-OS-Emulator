//
// Created by paren on 22/05/2025.
//

#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

class Process {
public:

    Process(int id, const std::string& name);


    int getID() const;
    const std::string& getName() const;
    const std::vector<std::string>& getLogs() const;
    int getCurrentLine() const;
    int getTotalLines() const;
    const std::string& getTimestamp() const;


    void log(const std::string& entry);
    void incrementLine();

private:
    int processID;
    std::string processName;
    std::vector<std::string> logs;
    int currentLine;
    int totalLines;
    std::string timestamp;

    std::string generateTimestamp() const;
};
