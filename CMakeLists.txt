cmake_minimum_required(VERSION 3.10)
project(sysprobe VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set linux env
set(CMAKE_SYSTEM_NAME Linux)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find required packages
find_package(Threads REQUIRED)

# Find ncurses for Phase 6 TUI (optional)
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(NCURSES ncurses)
endif()

# Fallback to find_package for ncurses
if(NOT NCURSES_FOUND)
    find_package(Curses QUIET)
    if(Curses_FOUND)
        set(NCURSES_LIBRARIES ${CURSES_LIBRARIES})
        set(NCURSES_INCLUDE_DIRS ${CURSES_INCLUDE_DIRS})
        set(NCURSES_FOUND TRUE)
    endif()
endif()

# Include directories
include_directories(include)
if(NCURSES_FOUND)
    include_directories(${NCURSES_INCLUDE_DIRS})
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/CpuMonitor.cpp
    src/MemoryMonitor.cpp
    src/StorageMonitor.cpp
)

# Advanced source files (all phases)
set(ADVANCED_SOURCES
    src/advanced_main.cpp
    src/CpuMonitor.cpp
    src/MemoryMonitor.cpp
    src/StorageMonitor.cpp
    src/PerfMonitor.cpp
    src/NumaMonitor.cpp
    src/ProcessMonitor.cpp
    src/AdvancedTUI.cpp
)

# Create executables
add_executable(sysprobe ${SOURCES})

# Create advanced executable without TUI for now
set(ADVANCED_SOURCES_NO_TUI
    src/advanced_main.cpp
    src/CpuMonitor.cpp
    src/MemoryMonitor.cpp
    src/StorageMonitor.cpp
    src/PerfMonitor.cpp
    src/NumaMonitor.cpp
    src/ProcessMonitor.cpp
)

add_executable(sysprobe-advanced ${ADVANCED_SOURCES_NO_TUI})

# Link libraries
target_link_libraries(sysprobe 
    Threads::Threads
)

target_link_libraries(sysprobe-advanced 
    Threads::Threads
)

# Installation
install(TARGETS sysprobe DESTINATION bin)

