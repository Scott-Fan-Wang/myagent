cmake_minimum_required(VERSION 3.10)
project(CodeAgent VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# Find required packages
find_package(Threads REQUIRED)

# Add executable
add_executable(code_agent
    main.cpp
    code_agent.cpp
)

# Link libraries
target_link_libraries(code_agent
    PRIVATE
    Threads::Threads
    stdc++fs  # For filesystem support (required for some older compilers)
)

# Install target
install(TARGETS code_agent DESTINATION bin)

# Print build information
message(STATUS "Building CodeAgent C++ Implementation")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
