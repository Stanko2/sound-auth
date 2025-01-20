cmake_minimum_required(VERSION 3.29)
project(sound-auth)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(SDL2 REQUIRED)


add_executable(sound-auth src/main.cpp)

add_subdirectory(external/ggwave)


set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(SOURCE_DIR  ${PROJECT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

add_library(pam_sound_auth SHARED ${SOURCE_DIR}/pam/authenticate.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES} ggwave)

target_compile_options(${PROJECT_NAME} PRIVATE ${SDL2_CFLAGS_OTHER})

target_compile_options(${PROJECT_NAME} PRIVATE -g)

install(TARGETS pam_sound_auth LIBRARY DESTINATION /lib/security)
