cmake_minimum_required(VERSION 3.10.0)
project(os_emulator VERSION 0.1.0 LANGUAGES C CXX)

add_executable(os_emulator src/main.cpp
        src/ConsoleManager.cpp
        src/ConsoleManager.h
        src/Process.cpp
        src/Process.h
        src/Screen.h
        src/MainScreen.h
        src/MainScreen.cpp
        src/ProcessScreen.h
        src/ProcessScreen.cpp
        src/ProcessScheduler.cpp
        src/ProcessScheduler.h
        src/Instruction.h
        src/PrintInstruction.h
        src/Instruction.cpp
        src/PrintInstruction.cpp
        src/Config.cpp
        src/Config.h
        src/SleepInstruction.cpp
        src/SleepInstruction.h
        src/DeclareInstruction.cpp
        src/DeclareInstruction.h
        src/ArithmeticInstruction.cpp
        src/ArithmeticInstruction.h
)

set_property(TARGET os_emulator PROPERTY CXX_STANDARD 23)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(os_emulator PRIVATE "-lstdc++exp")
endif ()