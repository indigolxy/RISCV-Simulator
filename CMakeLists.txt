cmake_minimum_required(VERSION 3.22)
project(RISCV-Simulator)

set(CMAKE_CXX_STANDARD 14)

add_executable(code src/main/main.cpp
        src/units/instruction.cpp
        src/main/cpu.cpp)
