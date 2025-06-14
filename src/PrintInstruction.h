#pragma once

#include "Instruction.h"
#include <string>
#include <string_view>
#include <concepts>

class PrintInstruction final : public Instruction {
private:
    std::string message;

public:
    template<typename T>
    requires std::convertible_to<T, std::string_view>
    explicit PrintInstruction(T&& msg);

    void execute() override;

    [[nodiscard]] const std::string& getMessage() const noexcept;
};

// Template implementation must be in header
template<typename T>
requires std::convertible_to<T, std::string_view>
PrintInstruction::PrintInstruction(T&& msg) : Instruction(1), message(std::forward<T>(msg)) {
}