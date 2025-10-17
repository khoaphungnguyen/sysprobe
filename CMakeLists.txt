cmake_minimum_required(VERSION 3.10)
project(tiny_monitor VERSION 0.1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/CpuMonitor.cpp
)

# Create executable
add_executable(tiny_monitor ${SOURCES})

# Link libraries
target_link_libraries(tiny_monitor 
    Threads::Threads
)

# Installation
install(TARGETS tiny_monitor DESTINATION bin)

