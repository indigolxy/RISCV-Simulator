cmake_minimum_required(VERSION 3.22)
project(RISCV-Simulator)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-g -Ofast")

add_executable(code src/main/main.cpp
        src/units/instruction.cpp
        src/main/cpu.cpp
        src/units/rss.cpp
        src/storage/lsb.cpp)
