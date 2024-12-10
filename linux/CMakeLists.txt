cmake_minimum_required(VERSION 3.29)
project(sound-auth)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(SDL2 REQUIRED)


add_executable(sound-auth main.cpp)

add_subdirectory(external/ggwave)

target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES} ggwave)

target_compile_options(${PROJECT_NAME} PRIVATE ${SDL2_CFLAGS_OTHER})
