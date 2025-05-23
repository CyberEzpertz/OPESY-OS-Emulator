#pragma once

#include "Screen.h"
#include "Process.h"
#include <memory>
#include <vector>
#include <string>

class ProcessScreen : public Screen {
public:
    explicit ProcessScreen(std::shared_ptr<Process> process);

    void render() override;
    void handleUserInput() override;
    std::string getName() const override;

private:
    std::shared_ptr<Process> processPtr;
};
