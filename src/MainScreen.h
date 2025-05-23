#pragma once

#include "Screen.h"
#include <string>
#include <vector>

class MainScreen final : public Screen {
public:
    MainScreen& operator=(const MainScreen&) = delete;

    static MainScreen getInstance();

    void render() override;
    void handleUserInput() override;
    std::string getName() const override;

private:
    MainScreen() = default;

    static void setColor(int color);
    static void resetColor();
    static void clrScreen();
    static void printHeader();
    static void printPlaceholder(const std::string& command);
    std::vector<std::string> splitInput(const std::string& input);
};
