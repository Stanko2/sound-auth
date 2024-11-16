cmake_minimum_required(VERSION 3.29)
project(sound-auth)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)

include_directories(${PIPEWIRE_INCLUDE_DIRS})

add_executable(sound-auth main.cpp)

target_link_libraries(sound-auth ${PIPEWIRE_LIBRARIES})

target_compile_options(sound-auth PRIVATE ${PIPEWIRE_CFLAGS_OTHER})
