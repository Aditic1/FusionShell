cmake_minimum_required(VERSION 3.10)
project(FusionShell)

set(CMAKE_CXX_STANDARD 17) # Updated to C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/fusionshell.cpp
    src/config.cpp
    src/parser.cpp
    src/executor.cpp
    src/job_control.cpp
    src/main.cpp
)

# Create executable
add_executable(fusionshell ${SOURCES})

# Add -fPIC to avoid DT_TEXTREL warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
