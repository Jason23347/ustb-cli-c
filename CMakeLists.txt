cmake_minimum_required(VERSION 3.10)
project(ustb-cli VERSION 1.2.1)

#---------------------------------------
# Build types
#---------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "-Wall -Wno-unused-variable")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O3 -s -w -flto -ffast-math")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-DNDEBUG -g -O2")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h
)

#---------------------------------------
# Options
#---------------------------------------
option(WITH_COLOR "Enable colorful output" ON)
option(WITH_ACCOUNT "Enable account features: login, logout, whoami" ON)
option(WITH_BALANCE "Enable balance features: info, fee" ON)
option(WITH_SPEEDTEST "Enable speedtest features: speedtest, monitor" ON)
option(WITH_COMPLETION "Enable completion features: completion" OFF)
set(GB2312_DECODER "iconv" CACHE STRING "Select GB2312 decoding method: iconv, builtin, or disabled")
set_property(CACHE GB2312_DECODER PROPERTY STRINGS "iconv" "builtin" "disabled")

message(STATUS "GB2312 decoder selected: ${GB2312_DECODER}")

if(GB2312_DECODER STREQUAL "iconv")
    add_definitions(-DUSE_ICONV)
elseif(GB2312_DECODER STREQUAL "builtin")
    add_definitions(-DUSE_BUILTIN_DECODER)
elseif(GB2312_DECODER STREQUAL "disabled")
    add_definitions(-DGB2312_DECODER_DISABLED)
else()
    message(FATAL_ERROR "Invalid GB2312_DECODER value: ${GB2312_DECODER}. Must be one of: iconv, builtin, disabled.")
endif()


#---------------------------------------
# Compiler settings
#---------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

#---------------------------------------
# Source files
#---------------------------------------
file(GLOB SRC_FILES
    lib/*.c
    src/*.c
    src/calc/*.c
    src/cmd/*.c
    src/net/*.c
)

add_executable(ustb-cli ${SRC_FILES})

target_include_directories(ustb-cli PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

IF(USE_ICONV)
find_package(Iconv REQUIRED)
ENDIF()
find_package(Threads REQUIRED)
target_link_libraries(ustb-cli PRIVATE Threads::Threads)

#---------------------------------------
# Subdirectories
#---------------------------------------
add_subdirectory(external/cargs)
target_link_libraries(ustb-cli PRIVATE cargs)
target_include_directories(ustb-cli PRIVATE external/cargs/include)
