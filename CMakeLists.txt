cmake_minimum_required(VERSION 3.14)
project(GomokuMCTS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags for performance
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(ENGINE_SOURCES
    src/board.cpp
    src/heuristic.cpp
    src/mcts.cpp
    src/uci.cpp
)

# Library
add_library(gomoku_engine STATIC ${ENGINE_SOURCES})

# Main executable
add_executable(gomoku src/main.cpp)
target_link_libraries(gomoku gomoku_engine)

# Test executable
add_executable(test_engine tests/test_engine.cpp)
target_link_libraries(test_engine gomoku_engine)

# Enable testing
enable_testing()
add_test(NAME GomokuTests COMMAND test_engine)

# Installation
install(TARGETS gomoku DESTINATION bin)
