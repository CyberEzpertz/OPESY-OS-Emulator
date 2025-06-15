#pragma once

#include <concepts>
#include <memory>
#include <string>
#include <string_view>

#include "Instruction.h"
#include "Process.h"

class PrintInstruction final : public Instruction {
private:
    std::string message;

public:
    PrintInstruction(const std::string& msg, std::shared_ptr<Process> process);

    void execute() override;

    [[nodiscard]] const std::string& getMessage() const noexcept;
};
