cmake_minimum_required(VERSION 2.6...3.10)
project(sound-auth)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(SDL2 REQUIRED)
find_package(libconfig REQUIRED)
find_package(OpenSSL REQUIRED)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)

set(SOURCES
    ${SOURCE_DIR}/audio/audio-control.cpp
    ${SOURCE_DIR}/audio/communication.cpp
    ${SOURCE_DIR}/ggwave/ggwave.cpp
    ${SOURCE_DIR}/config.cpp
)


add_library(pam_sound_auth SHARED
    ${SOURCE_DIR}/pam/authenticate.cpp
    ${SOURCES}
)
target_include_directories(pam_sound_auth PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(pam_sound_auth PRIVATE ${SDL2_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto config++)
target_compile_options(pam_sound_auth PRIVATE ${SDL2_CFLAGS_OTHER} -g)

add_executable(${PROJECT_NAME} src/main.cpp ${SOURCE_DIR}/ggwave/ggwave.cpp ${SOURCE_DIR}/config.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto config++)
target_compile_options(${PROJECT_NAME} PRIVATE ${SDL2_CFLAGS_OTHER} -g)

install(TARGETS pam_sound_auth LIBRARY DESTINATION /lib/security)
