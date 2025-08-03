#include "SleepInstruction.h"

#include "ConsoleManager.h"
#include "ProcessScheduler.h"

SleepInstruction::SleepInstruction(const uint8_t ticks, const int pid) : Instruction(1, pid), ticks(ticks) {
    this->opCode = "SLEEP";
}

void SleepInstruction::execute() {
    const auto process = getProcess();

    const auto currentTicks = ProcessScheduler::getInstance().getTotalCPUTicks();
    const uint64_t wakeupTick = ticks + currentTicks;
    process->setWakeupTick(wakeupTick);
    process->setStatus(WAITING);

    ProcessScheduler::getInstance().sleepProcess(process);
}

std::string SleepInstruction::serialize() const {
    return std::format("SLEEP {} {}", ticks, pid);
}