cmake_minimum_required(VERSION 3.10.0)
project(os_emulator VERSION 0.1.0 LANGUAGES C CXX)

add_executable(os_emulator main.cpp)

set_property(TARGET os_emulator PROPERTY CXX_STANDARD 23)
