cmake_minimum_required(VERSION 3.10)
project(ustb-cli VERSION 1.3.6)
include(CheckFunctionExists)

#---------------------------------------
# Build types
#---------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS "-Wall -Werror=implicit-function-declaration")
set(CMAKE_C_FLAGS_DEBUG "-g -O0")
set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -Wno-unused-variable -O3 -s -w -flto -ffast-math")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-DNDEBUG -g -O2")

# Check for functions before generating config.h
check_function_exists(strcasestr HAVE_STRCASESTR)

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
option(USE_INTERACTIVE "Enable interactive input with linenoise" OFF)
option(USE_THREADS "Enable multi-threaded speedtest implementation" ON)
set(GB2312_DECODER "iconv" CACHE STRING "Select GB2312 decoding method: iconv, builtin, or disabled")
set_property(CACHE GB2312_DECODER PROPERTY STRINGS "iconv" "builtin" "disabled")
set(OPENSSL_ROOT_DIR "" CACHE PATH "OpenSSL root directory (optional)")

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
    lib/compat_strcasestr.c
    lib/decode.c
    lib/gbuff.c
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

IF(USE_THREADS)
find_package(Threads REQUIRED)
target_link_libraries(ustb-cli PRIVATE Threads::Threads)
ENDIF()

target_link_libraries(ustb-cli PRIVATE m)


#---------------------------------------
# OpenSSL
#---------------------------------------
set(OpenSSL_FOUND FALSE)

if(OPENSSL_ROOT_DIR)
    # 用户指定了 OpenSSL 路径，使用 pkg-config 查找
    set(ENV{PKG_CONFIG_PATH} "${OPENSSL_ROOT_DIR}/lib64/pkgconfig:${OPENSSL_ROOT_DIR}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(OPENSSL libssl libcrypto)
        if(OPENSSL_FOUND)
            message(STATUS "Found OpenSSL via pkg-config: ${OPENSSL_ROOT_DIR}")
            message(STATUS "  Libraries: ${OPENSSL_LIBRARIES}")
            message(STATUS "  Include dirs: ${OPENSSL_INCLUDE_DIRS}")
            message(STATUS "  Library dirs: ${OPENSSL_LIBRARY_DIRS}")
            add_definitions(-DWITH_SSL)
            # Always include the OpenSSL include directory, even if pkg-config doesn't report it
            target_include_directories(ustb-cli PRIVATE ${OPENSSL_ROOT_DIR}/include ${OPENSSL_INCLUDE_DIRS})
            target_link_libraries(ustb-cli PRIVATE ${OPENSSL_LIBRARIES})
            target_link_directories(ustb-cli PRIVATE ${OPENSSL_ROOT_DIR}/lib64 ${OPENSSL_ROOT_DIR}/lib ${OPENSSL_LIBRARY_DIRS})
            set(OpenSSL_FOUND TRUE)
        else()
            message(WARNING "OpenSSL not found via pkg-config at ${OPENSSL_ROOT_DIR}, trying manual search...")
        endif()
    endif()

    # 如果pkg-config失败，使用手动查找
    if(NOT OpenSSL_FOUND)
        # Fallback to manual search
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
        # 同时查找 lib 和 lib64 目录
        set(OPENSSL_LIB_PATHS
            "${OPENSSL_ROOT_DIR}/lib64"
            "${OPENSSL_ROOT_DIR}/lib"
        )
        find_library(OPENSSL_SSL_LIBRARY
            NAMES ssl libssl ssl3 libssl3
            PATHS ${OPENSSL_LIB_PATHS}
            NO_DEFAULT_PATH
        )
        find_library(OPENSSL_CRYPTO_LIBRARY
            NAMES crypto libcrypto crypto3 libcrypto3
            PATHS ${OPENSSL_LIB_PATHS}
            NO_DEFAULT_PATH
        )
        find_path(OPENSSL_INCLUDE_DIR
            NAMES openssl/ssl.h
            PATHS "${OPENSSL_ROOT_DIR}/include"
            NO_DEFAULT_PATH
        )
        if(OPENSSL_SSL_LIBRARY AND OPENSSL_CRYPTO_LIBRARY AND OPENSSL_INCLUDE_DIR)
            message(STATUS "Found OpenSSL via manual search: ${OPENSSL_ROOT_DIR}")
            message(STATUS "  SSL Library: ${OPENSSL_SSL_LIBRARY}")
            message(STATUS "  Crypto Library: ${OPENSSL_CRYPTO_LIBRARY}")
            message(STATUS "  Include Directory: ${OPENSSL_INCLUDE_DIR}")
            add_definitions(-DWITH_SSL)
            target_include_directories(ustb-cli PRIVATE ${OPENSSL_INCLUDE_DIR})
            target_link_libraries(ustb-cli PRIVATE ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
            set(OpenSSL_FOUND TRUE)
        else()
            message(WARNING "OpenSSL not found at ${OPENSSL_ROOT_DIR}, HTTPS support will be disabled")
            if(NOT OPENSSL_SSL_LIBRARY)
                message(WARNING "  SSL library not found in ${OPENSSL_LIB_PATHS}")
            endif()
            if(NOT OPENSSL_CRYPTO_LIBRARY)
                message(WARNING "  Crypto library not found in ${OPENSSL_LIB_PATHS}")
            endif()
            if(NOT OPENSSL_INCLUDE_DIR)
                message(WARNING "  Include directory not found at ${OPENSSL_ROOT_DIR}/include")
            endif()
        endif()
    endif()
else()
    find_package(OpenSSL QUIET)
    if(OpenSSL_FOUND)
        message(STATUS "Found OpenSSL: ${OPENSSL_VERSION}")
        message(STATUS "  SSL Library: ${OPENSSL_SSL_LIBRARY}")
        message(STATUS "  Crypto Library: ${OPENSSL_CRYPTO_LIBRARY}")
        message(STATUS "  Include Directory: ${OPENSSL_INCLUDE_DIR}")
        add_definitions(-DWITH_SSL)
        target_include_directories(ustb-cli PRIVATE ${OPENSSL_INCLUDE_DIR})
        target_link_libraries(ustb-cli PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    else()
        message(STATUS "OpenSSL not found, HTTPS support will be disabled")
        message(STATUS "OpenSSL not found, command devices will be disabled")
        message(STATUS "  To enable HTTPS, install OpenSSL or set OPENSSL_ROOT_DIR")
    endif()
endif()

#---------------------------------------
# Hash library
#---------------------------------------
# Use md5.c if no openssl, else use md5_openssl.c
if(OpenSSL_FOUND)
    target_sources(ustb-cli PRIVATE lib/md5_openssl.c)
    target_include_directories(ustb-cli PRIVATE ${OPENSSL_INCLUDE_DIR})
else()
    target_sources(ustb-cli PRIVATE lib/md5.c)
endif()

#---------------------------------------
# Subdirectories
#---------------------------------------
add_subdirectory(external/cargs)
target_link_libraries(ustb-cli PRIVATE cargs)
target_include_directories(ustb-cli PRIVATE external/cargs/include)

#---------------------------------------
# Linenoise for interactive input
#---------------------------------------
if(USE_INTERACTIVE)
    target_sources(ustb-cli PRIVATE external/linenoise/linenoise.c)
    target_include_directories(ustb-cli PRIVATE external/linenoise)
endif()
