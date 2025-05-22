//
// Created by paren on 22/05/2025.
//

#include "Process.h"

Process::Process(int id, const std::string& name)
    : processID(id), processName(name), currentLine(0), totalLines(50) {
    timestamp = generateTimestamp();
}

int Process::getID() const {
    return processID;
}

const std::string& Process::getName() const {
    return processName;
}

const std::vector<std::string>& Process::getLogs() const {
    return logs;
}

int Process::getCurrentLine() const {
    return currentLine;
}

int Process::getTotalLines() const {
    return totalLines;
}

const std::string& Process::getTimestamp() const {
    return timestamp;
}

void Process::log(const std::string& entry) {
    logs.push_back(entry);
}

void Process::incrementLine() {
    if (currentLine < totalLines) currentLine++;
}

std::string Process::generateTimestamp() const {
    auto now = std::chrono::system_clock::now();
    std::time_t timeT = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&timeT);

    std::ostringstream oss;
    oss << std::put_time(&local_tm, "%m/%d/%Y, %I:%M:%S %p");
    return oss.str();
}

