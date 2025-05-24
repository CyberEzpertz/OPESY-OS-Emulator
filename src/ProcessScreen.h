#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Process.h"
#include "Screen.h"

class ProcessScreen : public Screen {
public:
    explicit ProcessScreen(std::shared_ptr<Process> process);

    void render() override;
    void handleUserInput() override;

private:
    std::shared_ptr<Process> processPtr;
};
