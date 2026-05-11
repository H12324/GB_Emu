cmake_minimum_required(VERSION 3.10)

project(GameBoyEmu)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCE_FILES
    src/CPU.cpp
    src/PPU.cpp
    src/GB.cpp
    src/main.cpp
    src/CPU.h
    src/PPU.h
    src/GB.h
    src/opcode.h
)

include_directories(include src)

# Debug and Release build types
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type selected, default to Debug")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Possible values are: Debug, Release, RelWithDebInfo, MinSizeRel
endif()

# Debug specific flags
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG_MODE) # example debug flag
    message(STATUS "Debug build")
endif()

# Release specific flags
if(CMAKE_BUILD_TYPE MATCHES Release)
    message(STATUS "Release build")
endif()

add_executable(GB_Emu ${SOURCE_FILES})

enable_testing()

find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    set(BLARGG_CPU_INSTRS_ROM
        "${CMAKE_CURRENT_SOURCE_DIR}/roms/blargg/cpu_instrs/cpu_instrs.gb")

    if(EXISTS "${BLARGG_CPU_INSTRS_ROM}")
        add_test(
            NAME blargg_cpu_instrs
            COMMAND
                ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_blargg.py
                $<TARGET_FILE:GB_Emu>
                ${BLARGG_CPU_INSTRS_ROM}
                --timeout 30
        )
    else()
        message(STATUS "Blargg cpu_instrs ROM not found; skipping Blargg CTest target")
    endif()
endif()
