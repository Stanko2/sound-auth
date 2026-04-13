cmake_minimum_required(VERSION 2.6...3.10)
project(sound-testing)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
# find_package(SDL2 REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
find_package(FFTW3 REQUIRED)

set(SOURCE_DIR ${PROJECT_SOURCE_DIR})

set(SOURCES
    ${SOURCE_DIR}/main.cpp
)

add_subdirectory(lib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

list(APPEND SOURCES ${TRANSFER_LIB_SOURCES})

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PIPEWIRE_LIBRARIES} fftw3)
target_compile_options(${PROJECT_NAME} PRIVATE ${PIPEWIRE_CFLAGS_OTHER} -g)
