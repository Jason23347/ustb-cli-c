cmake_minimum_required(VERSION 3.10)
project(ustb-cli VERSION 1.0)

#---------------------------------------
# Build types
#---------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "-Wall -Wno-unused-variable")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-O3 -s -w -flto")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-g -O2")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

#---------------------------------------
# Options
#---------------------------------------
option(WITH_COLOR "Enable colorful output" OFF)
option(WITH_ACCOUNT "Enable account features: login, logout, whoami" ON)
option(WITH_BALANCE "Enable balance features: info, fee" OFF)
option(WITH_SPEEDTEST "Enable balance features: info, fee" OFF)

#---------------------------------------
# Compiler settings
#---------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

#---------------------------------------
# Source files
#---------------------------------------
file(GLOB SRC_FILES
    src/*.c
    src/calc/*.c
    src/cmd/*.c
    src/net/*.c
)

add_executable(ustb-cli ${SRC_FILES})

target_include_directories(ustb-cli PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

find_package(Threads REQUIRED)
target_link_libraries(ustb-cli PRIVATE Threads::Threads)

#---------------------------------------
# Subdirectories
#---------------------------------------
add_subdirectory(external/cargs)
target_link_libraries(ustb-cli PRIVATE cargs)
target_include_directories(ustb-cli PRIVATE external/cargs/include)
