cmake_minimum_required(VERSION 2.6...3.10)
project(sound-auth)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)

find_package(libconfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(FFTW3 REQUIRED)

add_subdirectory(src/sound-transfer-lib)


set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)

set(SOURCES
    ${SOURCE_DIR}/audio/audio-control.cpp
    ${SOURCE_DIR}/audio/communication.cpp
    ${SOURCE_DIR}/ggwave/ggwave.cpp
    ${SOURCE_DIR}/config.cpp
)

list(APPEND SOURCES ${TRANSFER_LIB_SOURCES})


add_library(pam_sound_auth SHARED
    ${SOURCE_DIR}/pam/authenticate.cpp
    ${SOURCES}
)
target_include_directories(pam_sound_auth PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
target_link_libraries(pam_sound_auth PRIVATE ${PIPEWIRE_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto)
target_compile_options(pam_sound_auth PRIVATE ${PIPEWIRE_CFLAGS_OTHER} -g)

add_executable(${PROJECT_NAME} src/main.cpp ${SOURCE_DIR}/ggwave/ggwave.cpp ${SOURCE_DIR}/config.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PIPEWIRE_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto)
target_compile_options(${PROJECT_NAME} PRIVATE ${PIPEWIRE_CFLAGS_OTHER} -g)

install(TARGETS pam_sound_auth LIBRARY DESTINATION /lib/security)
