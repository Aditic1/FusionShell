cmake_minimum_required(VERSION 3.10)
project(FusionShell)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSIVE SOURCES "src/*.cpp")

add_executable(fusionshell ${SOURCES})
