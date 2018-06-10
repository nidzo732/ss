cmake_minimum_required(VERSION 3.5)
project(ss)

set(CMAKE_CXX_STANDARD 14)
find_package (Threads)

add_executable(ssas as_main.cpp assembler/Line.cpp assembler/Line.h assembler/Operand.cpp assembler/Operand.h assembler/File.cpp assembler/File.h assembler/Assembler.cpp assembler/Assembler.h common/Symbol.cpp common/Symbol.h common/machine_params.h common/RelocationEntry.cpp common/RelocationEntry.h)
add_executable(ssemu emu_main.cpp common/Symbol.cpp common/Symbol.h common/machine_params.h common/RelocationEntry.cpp common/RelocationEntry.h emulator/Memory.cpp emulator/Memory.h emulator/Machine.cpp emulator/Machine.h emulator/Instruction.cpp emulator/Instruction.h emulator/File.h emulator/File.cpp)

target_link_libraries (ssemu ${CMAKE_THREAD_LIBS_INIT})