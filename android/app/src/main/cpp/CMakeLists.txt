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

set(TRANSFER_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../tests/audio/lib/)

file(COPY ${TRANSFER_LIB_DIR} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/transfer-lib/)


add_subdirectory(transfer-lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/transfer-lib)


# List all source files for the native library
set(SOURCES
        main.cpp
        ggwave/ggwave.cpp
        transfer.cpp
        ${TRANSFER_LIB_SOURCES}
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

find_library(aaudio-lib aaudio)

# Link Android libraries
target_link_libraries(
        soundAuth                       # The native library
        PRIVATE
        ${log-lib}                      # Link to Android log library
        ${aaudio-lib}
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        fftw3f
)
