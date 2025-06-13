#pragma once

#include "Instruction.h"

class PrintInstruction final : public Instruction {
private:
    std::string message;

public:
    template<typename T>
    requires std::convertible_to<T, std::string_view>
    explicit PrintInstruction(T&& msg) : Instruction(1), message(std::forward<T>(msg)) {}

    void execute(Process& process) override {
        process.logs.emplace_back(message);
    }

    [[nodiscard]] const std::string& getMessage() const noexcept {
        return message;
    }
};