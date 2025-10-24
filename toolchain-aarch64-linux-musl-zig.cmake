# =============================================================================
# Advanced System Monitor - AArch64 Linux Musl Cross-Compilation Toolchain
# =============================================================================
# This toolchain enables cross-compilation of the advanced system monitor
# (Phases 3-6) for AArch64 Linux systems using Zig as the compiler frontend
# and musl libc for static linking and portability.
# =============================================================================

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# =============================================================================
# Compiler Configuration
# =============================================================================
# Use Zig as the compiler front-end for cross compilation
set(CMAKE_C_COMPILER zig)
set(CMAKE_CXX_COMPILER zig)
set(CMAKE_C_COMPILER_ARG1 cc)
set(CMAKE_CXX_COMPILER_ARG1 c++)

# Tell Zig to target Linux+aarch64 with musl libc
set(CMAKE_C_COMPILER_TARGET   aarch64-linux-musl)
set(CMAKE_CXX_COMPILER_TARGET aarch64-linux-musl)

# =============================================================================
# Cross-Compilation Settings
# =============================================================================
# Ensure CMake does not try to run test executables on the host when probing
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Prefer static linking for a portable binary
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static")

# Make CMake search paths target-aware (avoid picking macOS host headers/libs)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# =============================================================================
# Advanced System Monitor Specific Configuration
# =============================================================================

# Enable C++17 features required for advanced monitoring
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for advanced system monitoring
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -DRELEASE")

# =============================================================================
# Linux-Specific Features for Advanced Monitoring
# =============================================================================

# Enable Linux-specific features for hardware performance counters
add_definitions(-D__linux__)
add_definitions(-D_GNU_SOURCE)

# Enable advanced system monitoring features
add_definitions(-DADVANCED_MONITORING=1)
add_definitions(-DPERF_EVENTS_ENABLED=1)
add_definitions(-DNUMA_MONITORING=1)
add_definitions(-DPROCESS_MONITORING=1)

# =============================================================================
# Library Configuration
# =============================================================================

# Thread library is provided by musl; keep Threads::Threads usage in CMakeLists
# Note: For advanced TUI features, ncurses would need to be statically linked
# This toolchain focuses on the core monitoring capabilities (Phases 3-5)

# =============================================================================
# Build Configuration for Advanced Features
# =============================================================================

# Enable all advanced monitoring phases
set(ENABLE_PHASE_3_PERF_COUNTERS ON)
set(ENABLE_PHASE_4_NUMA_ANALYSIS ON)
set(ENABLE_PHASE_5_PROCESS_MONITORING ON)
set(ENABLE_PHASE_6_TUI OFF)  # TUI requires ncurses, disabled for static build

# =============================================================================
# Performance Optimization
# =============================================================================

# Optimize for system monitoring performance
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ffast-math -funroll-loops")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer")

# =============================================================================
# Static Linking Configuration
# =============================================================================

# Ensure all dependencies are statically linked for portability
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)

# =============================================================================
# Advanced System Monitor Build Notes
# =============================================================================
# This toolchain enables building the advanced system monitor with:
# - Phase 3: Hardware performance counters (perf_event_open)
# - Phase 4: NUMA topology and memory pressure analysis
# - Phase 5: Process-level monitoring and resource attribution
# - Phase 6: TUI disabled (requires ncurses for full functionality)
#
# To build:
#   mkdir build-aarch64 && cd build-aarch64
#   cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-aarch64-linux-musl-zig.cmake ..
#   make
#
# The resulting binary will be statically linked and portable across
# AArch64 Linux systems with musl libc.
# =============================================================================
