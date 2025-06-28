# OS Project (Group 9)

The main objective of this project is to simulate a mini operating system using C++. This project uses **C++23**, *
*CMake**, and **Ninja**.

## Project Structure

```
/root
├── CMakeLists.txt
├── .clang-tidy
├── .clang-format
├── build/
└── src/
    ├── main.cpp
    ├── [class].cpp
    └── [class].h
```

### Description

- `src/`  
  Contains all source code files (`.cpp`, `.h`).
    - `main.cpp` — entry point of the application
    - Other source and header files, e.g., `ConsoleManager.cpp`, `ConsoleManager.h`

- `build/`  
  Directory for all generated build files and binaries (usually gitignored).

- `CMakeLists.txt`  
  Build configuration file for CMake.

- `.clang-tidy`  
  Configuration file for clang-tidy static analysis tool.

- `.clang-format`  
  Configuration file for clang-format code formatting tool.

## How to build and run

Run the following code inside powershell or cmd to build the application:

```sh
cmake --build build --target os_emulator -j 6
```

To run the actual application, either go to the build folder and run it manually, or run the following:

```sh
./build/os_emulator
```

## Group Members

- Murillo, Jan Anthony
- Caramat, Christian Jude
- Cheng, Ken
- Parente, Gabriel
