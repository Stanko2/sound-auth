cmake_minimum_required(VERSION 2.6...3.10)
project(sound-testing)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(SDL2 REQUIRED)
find_package(FFTW3 REQUIRED)

set(SOURCE_DIR ${PROJECT_SOURCE_DIR})

set(SOURCES
    ${SOURCE_DIR}/main.cpp
)


add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES} fftw3)
target_compile_options(${PROJECT_NAME} PRIVATE ${SDL2_CFLAGS_OTHER} -g)
