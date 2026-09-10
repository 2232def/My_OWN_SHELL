cmake_minimum_required(VERSION 3.15)
project(MyOwnShell VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add the executable
add_executable(myshell src/main.c)

# Compiler warnings
if(MSVC)
    target_compile_options(myshell PRIVATE /W4)
else()
    target_compile_options(myshell PRIVATE -Wall -Wextra -pedantic)
endif()

# Print build info
message(STATUS "Building ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
