#include "SleepInstruction.h"

#include "ConsoleManager.h"
#include "ProcessScheduler.h"

SleepInstruction::SleepInstruction(const uint8_t ticks, const int pid)
    : Instruction(1, pid), ticks(ticks) {
}

void SleepInstruction::execute() {
    const auto process = getProcess();

    const auto currentTicks = ProcessScheduler::getInstance().getCurrentCycle();
    const uint64_t wakeupTick = ticks + currentTicks;
    process->setWakeupTick(wakeupTick);
    process->setStatus(WAITING);

    ProcessScheduler::getInstance().sleepProcess(process);
}