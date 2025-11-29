# Specify the minimum required CMake version
cmake_minimum_required(VERSION 3.10.2)

include(lib/openssl/android_openssl.cmake)

set(OPENSSL_ROOT_DIR ${CMAKE_SOURCE_DIR}/lib/openssl/ssl_3/${ANDROID_ABI})
include_directories(${OPENSSL_ROOT_DIR}/include)

set(OPENSSL_SSL_LIBRARY ${OPENSSL_ROOT_DIR}/libssl.a)
set(OPENSSL_CRYPTO_LIBRARY ${OPENSSL_ROOT_DIR}/libcrypto.a)


add_library(fftw3f STATIC IMPORTED)
set_target_properties(fftw3f PROPERTIES
        IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/fftw3/${ANDROID_ABI}/libfftw3f.a"
)
include_directories("${CMAKE_SOURCE_DIR}/lib/fftw3/include")  # adjust path

# Define the project
project(soundAuth CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# List all source files for the native library
set(SOURCES
        main.cpp
        ggwave.cpp
        transfer.cpp
)

if (ANDROID)
    add_android_openssl_libraries(soundAuth)
endif()


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
        PRIVATE
        ${log-lib}                      # Link to Android log library
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        fftw3f
)
