#include "MainScreen.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <print>
#include <ranges>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "ConsoleManager.h"
#include "PagingAllocator.h"
#include "ProcessScheduler.h"

/// @brief Returns the singleton instance of MainScreen.
/// @return A single shared instance of MainScreen.
MainScreen& MainScreen::getInstance() {
    static MainScreen instance;
    return instance;
}

/// @brief Renders the main screen header and starts handling user input.
void MainScreen::render() {
    const std::string asciiArt =
        R"(
__________                         .__       ________    _________
\______   \ _______  __ ___________|__| ____ \_____  \  /   _____/
 |       _// __ \  \/ // __ \_  __ \  |/ __ \ /   |   \ \_____  \
 |    |   \  ___/\   /\  ___/|  | \/  \  ___//    |    \/        \
 |____|_  /\___  >\_/  \___  >__|  |__|\___  >_______  /_______  /
        \/     \/          \/              \/        \/        \/
)";

    setColor(35);
    std::print("{}", asciiArt);
    resetColor();

    std::println("{:->60}", "");
    setColor(36);
    std::println("Hello, Welcome to the ReverieOS commandline!");
    resetColor();

    if (ConsoleManager::getInstance().getHasInitialized()) {
        setColor(33);
        std::println("Type 'exit' to quit, 'clear' to clear the screen");
        resetColor();
    } else {
        setColor(33);
        std::println("Type 'initialize' to start the program, exit' to quit");
        resetColor();
    }

    std::println("{:->60}", "");
}
///
/// @brief Processes a single user command from standard input.
///
/// Reads a line of input, normalizes whitespace, and parses it into tokens.
/// Executes commands like 'exit', 'clear', 'screen' options, and predefined
/// commands.
///
/// Recognized commands:
/// - "exit": Signals the ConsoleManager to exit the program loop.
/// - "clear": Clears the console screen and prints the header.
/// - "screen -s <name>": Placeholder for creating a new screen.
/// - "screen -r <name>": Placeholder for resuming a screen.
/// - "screen -ls": Displays the processes
/// - "scheduler-start", "scheduler-stop", "report-util", "initialize":
/// Placeholders for other commands.
///
/// If the command is unrecognized, prints an error message.
///
void MainScreen::handleUserInput() {
    std::string input;
    setColor(35);
    std::print("reverie-✦> ");
    resetColor();

    std::getline(std::cin, input);

    // Normalize whitespace (collapse multiple spaces)
    std::istringstream iss(input);
    std::string word;
    std::vector<std::string> tokens;
    while (iss >> word) {
        tokens.push_back(word);
    }

    if (tokens.empty())
        return;

    const std::string& cmd = tokens[0];
    ConsoleManager& console = ConsoleManager::getInstance();

    if (!console.getHasInitialized()) {
        if (cmd == "exit") {
            console.exitProgram();
        } else if (cmd == "initialize") {
            console.initialize();
            std::println("Program initialized");
        } else {
            std::println("Error: Program has not been initialized. Please type "
                         "\"initialize\" before proceeding.");
        }
        return;
    }

    if (cmd == "exit") {
        console.exitProgram();  // Trigger outer loop exit
    } else if (cmd == "clear") {
        ConsoleManager::clearConsole();
        render();
    } else if (cmd == "screen") {
        handleScreenCommand(tokens);
    } else if (cmd == "initialize") {
        std::println("Program has already been initialized.");
    } else if (cmd == "scheduler-start") {
        ProcessScheduler& scheduler = ProcessScheduler::getInstance();
        if (scheduler.isGeneratingDummies()) {
            std::println("Dummy process generation is already running.");
            std::println("Use 'scheduler-stop' to stop it first.");
        } else {
            scheduler.startDummyGeneration();
        }
    } else if (cmd == "scheduler-stop") {
        ProcessScheduler& scheduler = ProcessScheduler::getInstance();
        if (!scheduler.isGeneratingDummies()) {
            std::println("Dummy process generation is not currently running.");
        } else {
            scheduler.stopDummyGeneration();
        }
    } else if (cmd == "scheduler-status") {
        // Optional: Add a status command for debugging
        ProcessScheduler& scheduler = ProcessScheduler::getInstance();
        std::println("Scheduler Status:");
        std::println("- CPU Cycles: {}", scheduler.getTotalCPUTicks());
        std::println("- Dummy Generation: {}", scheduler.isGeneratingDummies() ? "Running" : "Stopped");
        std::println("- Available Cores: {}/{}", scheduler.getNumAvailableCores(), scheduler.getNumTotalCores());
        scheduler.printQueues();
    } else if (cmd == "report-util") {
        generateUtilizationReport();
    } else if (cmd == "visualize") {
        PagingAllocator::getInstance().visualizeMemory();
    } else if (cmd == "process-smi") {
        generateProcessSMI();
    } else if (cmd == "vmstat") {
        generateVmStat();
    } else {
        std::println("Error: Unknown command {}", cmd);
    }
}

void MainScreen::handleScreenCommand(const std::vector<std::string>& tokens) {
    auto& console = ConsoleManager::getInstance();

    if (tokens.size() < 2) {
        std::println("Error: Not enough arguments for screen command.");
        return;
    }

    const std::string& flag = tokens[1];

    if (flag == "-ls") {
        if (tokens.size() > 2) {
            std::println("Error: Too many arguments for -ls.");
        } else {
            printProcessReport();
        }
        return;
    }

    if (flag == "-s" || flag == "-r") {
        if (tokens.size() < 3) {
            std::println("Error: Missing process name for {} flag.", flag);
            return;
        }

        const std::string& processName = tokens[2];

        if (flag == "-s") {
            if (console.createProcess(processName)) {
                console.switchConsole(processName);
            }
        } else {  // -r
            console.switchConsole(processName);
        }

        return;
    }

    if (flag == "-c") {
        if (tokens.size() < 5) {
            std::println("Error: screen -c requires <name> <mem_size> \"<instructions>\"");
            std::println("Usage: screen -c <name> <mem_size> \"<instrs;separated;by;semicolons>\"");
            return;
        }

        const std::string& processName = tokens[2];

        // Parse memory size
        int memSize;
        try {
            memSize = std::stoi(tokens[3]);
        } catch (const std::exception& e) {
            std::println("Error: Invalid memory size '{}'. Must be a number.", tokens[3]);
            return;
        }

        // Reconstruct instruction string from remaining tokens
        // This handles cases where the instruction string might have been split by spaces
        std::string instrStr;
        for (size_t i = 4; i < tokens.size(); ++i) {
            if (i > 4)
                instrStr += " ";
            instrStr += tokens[i];
        }

        // Remove surrounding quotes if present
        if (instrStr.size() >= 2 && instrStr.front() == '"' && instrStr.back() == '"') {
            std::istringstream iss(instrStr);
            std::string result;
            iss >> std::quoted(instrStr);
        }

        if (instrStr.empty()) {
            std::println("Error: No instructions provided.");
            return;
        }

        // Create the process with custom instructions
        if (console.createProcessWithCustomInstructions(processName, memSize, instrStr)) {
            // Optionally switch to the newly created process
            console.switchConsole(processName);
        }

        return;
    }

    // If the flag is not recognized
    std::println("Invalid screen flag: {}", flag);
}

/// @brief Sets the console text color using ANSI escape codes.
/// @param color The color code to apply.
void MainScreen::setColor(int color) {
    std::print("\033[{}m", color);
}

/// @brief Resets the console text color to the default.
void MainScreen::resetColor() {
    std::print("\033[0m");
}

/// @brief Prints a placeholder message for recognized commands not yet
/// implemented.
/// @param command The command to acknowledge.
void MainScreen::printPlaceholder(const std::string& command) {
    std::println("'{}' command recognized. Doing something.", command);
}

void MainScreen::printProcessReport() {
    const ProcessScheduler& scheduler = ProcessScheduler::getInstance();

    int availableCores = scheduler.getNumAvailableCores();
    int numCores = scheduler.getNumTotalCores();
    double cpuUtil = static_cast<double>(numCores - availableCores) / numCores * 100.0;

    auto processes = ConsoleManager::getInstance().getProcessNameMap();

    std::vector<std::shared_ptr<Process>> sorted;

    for (const auto& proc : processes | std::views::values) {
        sorted.push_back(proc);
    }

    std::ranges::sort(sorted, [](const auto& a, const auto& b) { return a->getName() < b->getName(); });

    std::println("CPU Utilization: {:.0f}%", cpuUtil);
    std::println("Cores used: {}", numCores - availableCores);
    std::println("Cores available: {}", availableCores);
    std::println("Total Cores: {}", numCores);

    std::println("{:->30}", "");

    std::println("Waiting Processes");

    for (const auto& process : sorted) {
        if (process->getStatus() == WAITING) {
            std::string coreStr = (process->getCurrentCore() == -1) ? "N/A" : std::to_string(process->getCurrentCore());

            std::println("{:<10}\t({:<8})\tCore:\t{:<4}\t{} / {}", process->getName(), process->getTimestamp(), coreStr,
                         process->getCurrentLine(), process->getTotalLines());
        }
    }

    std::println("Running processes:");

    for (const auto& process : sorted) {
        if (process->getStatus() != DONE && process->getStatus() != WAITING) {
            std::string coreStr = (process->getCurrentCore() == -1) ? "N/A" : std::to_string(process->getCurrentCore());

            std::println("{:<10}\t({:<8})\tCore:\t{:<4}\t{} / {}", process->getName(), process->getTimestamp(), coreStr,
                         process->getCurrentLine(), process->getTotalLines());
        }
    }

    std::println("\nFinished processes:");

    for (const auto& process : sorted) {
        if (process->getStatus() == DONE) {
            std::println("{:<10}\t({:<8})\tFinished\t{} / {}", process->getName(), process->getTimestamp(),
                         process->getCurrentLine(), process->getTotalLines());
        }
    }

    std::println("{:->30}", "");
}

/// @brief Generates and saves a CPU utilization report to csopesy-log.txt
void MainScreen::generateUtilizationReport() {
    try {
        ProcessScheduler& scheduler = ProcessScheduler::getInstance();

        int availableCores = scheduler.getNumAvailableCores();
        int numCores = scheduler.getNumTotalCores();
        double cpuUtil = static_cast<double>(numCores - availableCores) / numCores * 100.0;

        auto processes = ConsoleManager::getInstance().getProcessNameMap();

        std::vector<std::shared_ptr<Process>> sorted;
        for (const auto& proc : processes | std::views::values) {
            sorted.push_back(proc);
        }

        std::ranges::sort(sorted, [](const auto& a, const auto& b) { return a->getName() < b->getName(); });

        // Generate timestamp
        auto now = std::chrono::system_clock::now();
        std::time_t timeT = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm = *std::localtime(&timeT);

        std::ostringstream timestamp;
        timestamp << std::put_time(&local_tm, "%m/%d/%Y, %I:%M:%S %p");

        // Ensure the logs directory exists
        std::filesystem::create_directories("logs");

        // Write to file in logs folder
        std::ofstream outFile("logs/csopesy-log.txt");
        if (!outFile.is_open()) {
            std::println("Error: Could not create logs/csopesy-log.txt file.");
            return;
        }

        outFile << "CPU Utilization Report\n";
        outFile << "Timestamp: " << timestamp.str() << "\n\n";
        outFile << "CPU Utilization: " << std::fixed << std::setprecision(0) << cpuUtil << "%\n";
        outFile << "Cores used: " << numCores - availableCores << "\n";
        outFile << "Cores available: " << availableCores << "\n";
        outFile << "Total Cores: " << numCores << "\n\n";

        outFile << "------------------------------\n\n";

        outFile << "Waiting Processes:\n";
        for (const auto& process : sorted) {
            if (process->getStatus() == WAITING) {
                std::string coreStr =
                    (process->getCurrentCore() == -1) ? "N/A" : std::to_string(process->getCurrentCore());

                outFile << process->getName() << "\t(" << process->getTimestamp() << ")\tCore:\t" << coreStr << "\t"
                        << process->getCurrentLine() << " / " << process->getTotalLines() << "\n";
            }
        }

        outFile << "\nRunning processes:\n";
        for (const auto& process : sorted) {
            if (process->getStatus() != DONE && process->getStatus() != WAITING) {
                std::string coreStr =
                    (process->getCurrentCore() == -1) ? "N/A" : std::to_string(process->getCurrentCore());

                outFile << process->getName() << "\t(" << process->getTimestamp() << ")\tCore:\t" << coreStr << "\t"
                        << process->getCurrentLine() << " / " << process->getTotalLines() << "\n";
            }
        }

        outFile << "\nFinished processes:\n";
        for (const auto& process : sorted) {
            if (process->getStatus() == DONE) {
                outFile << process->getName() << "\t(" << process->getTimestamp() << ")\tFinished\t"
                        << process->getCurrentLine() << " / " << process->getTotalLines() << "\n";
            }
        }

        outFile << "\n------------------------------\n";
        outFile.close();

        std::println("Report generated at logs/csopesy-log.txt");

    } catch (const std::exception& e) {
        std::println("Error generating report: {}", e.what());
    }
}

void MainScreen::generateProcessSMI() {
    const ProcessScheduler& scheduler = ProcessScheduler::getInstance();
    const PagingAllocator& allocator = PagingAllocator::getInstance();

    const int availableCores = scheduler.getNumAvailableCores();
    const int numCores = scheduler.getNumTotalCores();
    double cpuUtil = static_cast<double>(numCores - availableCores) / numCores * 100.0;

    const auto usedMem = allocator.getUsedMemory();
    const auto totalMem = Config::getInstance().getMaxOverallMem();
    const auto memUtil = static_cast<double>(usedMem) / totalMem * 100.0;

    const auto coreAssignments = scheduler.getCoreAssignments();
    constexpr std::string_view header = "| PROCESS-SMI V01.00 Driver Version: 01.00 |";

    setColor(36);  // Cyan
    std::println("{}", std::string(header.length(), '-'));
    setColor(1);  // Bold
    std::println(header);
    resetColor();
    setColor(36);
    std::println("{}", std::string(header.length(), '-'));
    resetColor();

    setColor(32);  // Green
    std::print("CPU-Util: ");
    setColor(33);  // Yellow
    std::println("{:.0f}%", cpuUtil);

    setColor(32);
    std::print("Memory Usage: ");
    setColor(33);
    std::println("{}B / {}B", usedMem, totalMem);

    setColor(32);
    std::print("Memory Util: ");
    setColor(33);
    std::println("{:.0f}%", memUtil);
    resetColor();

    setColor(36);
    std::println("{}", std::string(header.length(), '='));
    resetColor();

    setColor(1);
    std::println("Running processes and memory usage:");
    resetColor();

    setColor(2);  // Dim
    std::println("{}", std::string(header.length(), '-'));
    resetColor();

    std::set<int> seenProcessIds;
    int coreIndex = 0;

    for (const auto& process : coreAssignments) {
        if (!process)
            continue;

        setColor(36);  // Cyan
        std::print("Core {:<2}:  ", coreIndex);
        resetColor();

        std::print("{:<12} {:<8} ", process->getName(), process->getMemoryUsage());

        if (process->getStatus() == RUNNING) {
            setColor(32);  // Green
            std::println("RUNNING");
        } else {
            setColor(33);  // Yellow
            std::println("OTHER");
        }

        resetColor();
        seenProcessIds.insert(process->getID());
        ++coreIndex;
    }

    setColor(2);  // Dim
    std::println("{}", std::string(header.length(), '-'));
    resetColor();

    setColor(1);
    std::println("Ready/Waiting processes with memory usage:");
    resetColor();

    setColor(2);  // Dim
    std::println("{}", std::string(header.length(), '-'));
    resetColor();

    for (const auto& process : ConsoleManager::getInstance().getProcessIdList()) {
        if (!process || seenProcessIds.contains(process->getID()))
            continue;

        const auto status = process->getStatus();
        if ((status != WAITING && status != READY) || process->getMemoryUsage() <= 0)
            continue;

        if (status == WAITING) {
            setColor(35);  // Magenta
            std::print("WAITING  ");
        } else {
            setColor(34);  // Blue
            std::print("READY    ");
        }

        resetColor();
        std::println("{:<12} {:<8}B", process->getName(), process->getMemoryUsage());
    }

    setColor(2);  // Dim
    std::println("{}", std::string(header.length(), '-'));
    resetColor();
}

void MainScreen::generateVmStat() {
    const ProcessScheduler& scheduler = ProcessScheduler::getInstance();
    const PagingAllocator& allocator = PagingAllocator::getInstance();

    const int usedMem = allocator.getUsedMemory();
    const int totalMem = Config::getInstance().getMaxOverallMem();
    const int freeMem = totalMem - usedMem;

    const uint64_t idleTicks = scheduler.getIdleCPUTicks();
    const uint64_t activeTicks = scheduler.getActiveCPUTicks();
    const uint64_t totalTicks = scheduler.getTotalCPUTicks();

    const int numPagedIn = allocator.getNumPagedIn();
    const int numPagedOut = allocator.getNumPagedOut();

    std::println("\n===== System Statistics =====");
    std::println("{:>20} {}", totalMem, "B Total memory");
    std::println("{:>20} {}", usedMem, "B Used memory");
    std::println("{:>20} {}", freeMem, "B Free memory");

    std::println("{:>20} {}", idleTicks, "Idle CPU ticks");
    std::println("{:>20} {}", activeTicks, "Active CPU ticks");
    std::println("{:>20} {}", totalTicks, "Total CPU ticks");

    std::println("{:>20} {}", numPagedIn, "Pages paged in");
    std::println("{:>20} {}", numPagedOut, "Pages paged out");

    std::println("==============================\n");
}