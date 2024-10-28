# Specify the minimum required CMake version
cmake_minimum_required(VERSION 3.10.2)

# Define the project
project(soundAuth)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# List all source files for the native library
set(SOURCES
        main.cpp
        ggwave.cpp

)

# Specify the library to be built
add_library(
        soundAuth                 # The name of the native library
        SHARED                          # Shared library, suitable for Android
        ${SOURCES}                      # Add source files
)

# Include Android NDK libraries for compatibility
find_library(
        log-lib                         # Define the Android log library
        log
)

# Link Android libraries
target_link_libraries(
        soundAuth                       # The native library
        ${log-lib}                      # Link to Android log library
)
