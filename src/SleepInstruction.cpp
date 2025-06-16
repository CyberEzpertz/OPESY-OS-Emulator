//
// Created by Jan on 16/06/2025.
//

#include "SleepInstruction.h"

#include "ProcessScheduler.h"

SleepInstruction::SleepInstruction(const uint8_t ticks,
                                   const std::shared_ptr<Process>& process)
    : Instruction(1, process), ticks(ticks) {
}

void SleepInstruction::execute() {
    const auto currentTicks = ProcessScheduler::getInstance().getCurrentCycle();
    const uint64_t wakeupTick = ticks + currentTicks;
    process->setWakeupTick(wakeupTick);
    process->setStatus(WAITING);

    ProcessScheduler::getInstance().sleepProcess(process);
}