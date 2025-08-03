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
    std::string varName;

public:
    PrintInstruction(const std::string& msg, const int pid);
    PrintInstruction(const std::string& msg, const int pid, const std::string& varName);

    void execute() override;

    [[nodiscard]] const std::string& getMessage() const noexcept;
    std::string serialize() const override;
};
