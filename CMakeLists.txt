cmake_minimum_required(VERSION 3.10)
project(ustb-cli VERSION 1.3.9)
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

string(TIMESTAMP CONFIGURE_TIME "%Y-%m-%d %H:%M:%S" UTC)

set(GIT_COMMIT_HASH "")
set(GIT_TAG "")
set(VERSION_DISPLAY "v${PROJECT_VERSION}")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND git describe --exact-match --tags HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(GIT_TAG)
        set(VERSION_DISPLAY "${GIT_TAG}")
    elseif(GIT_COMMIT_HASH)
        set(VERSION_DISPLAY "v${PROJECT_VERSION}+${GIT_COMMIT_HASH}")
    endif()
    # 检查是否有未提交的更改
    execute_process(
        COMMAND git status --porcelain
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_STATUS_PORCELAIN
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(GIT_STATUS_PORCELAIN)
        set(VERSION_DISPLAY "${VERSION_DISPLAY} (dirty)")
    endif()
endif()
if(NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "unknown")
endif()

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
# 四种情况:
# 1. 纯粹使用用户指定的 openssl: -DOPENSSL_ROOT_DIR=/path/to/openssl
# 2. 系统 openssl + 自定义 include: -DOPENSSL_INCLUDE_DIR=/path/to/headers
# 3. OpenWRT 交叉编译: 未设 OPENSSL_ROOT_DIR 时用 CMAKE_FIND_ROOT_PATH/usr
# 4. 纯系统 openssl: 不指定任何路径，使用系统默认的 lib 与 include
#---------------------------------------

option(OPENSSL_INCLUDE_DIR "Path to custom OpenSSL headers (optional)" "")
option(OPENSSL_ROOT_DIR "OpenSSL root directory (optional)" "")

set(OPENSSL_OK FALSE)

# OpenWRT: staging 里 OpenSSL 在 target-xxx/usr 下
if(NOT OPENSSL_ROOT_DIR AND CMAKE_FIND_ROOT_PATH)
    list(GET CMAKE_FIND_ROOT_PATH 0 _ROOT)
    set(OPENSSL_ROOT_DIR "${_ROOT}/usr")
endif()

if(OPENSSL_ROOT_DIR)
    set(ENV{PKG_CONFIG_PATH} "${OPENSSL_ROOT_DIR}/lib64/pkgconfig:${OPENSSL_ROOT_DIR}/lib/pkgconfig:${OPENSSL_ROOT_DIR}/usr/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    set(CMAKE_PREFIX_PATH "${OPENSSL_ROOT_DIR};${CMAKE_PREFIX_PATH}")
endif()

find_package(OpenSSL QUIET)

if(OpenSSL_FOUND)
    # 情况1 或 3: find_package 成功（来自 OPENSSL_ROOT_DIR 或 OpenWRT staging）
    set(OPENSSL_OK TRUE)
    message(STATUS "OpenSSL found via find_package")
    target_compile_definitions(ustb-cli PRIVATE WITH_SSL)
    target_link_libraries(ustb-cli PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    # 若用户额外指定了 include，则覆盖（用于头文件版本与库不一致等场景）
    if(OPENSSL_INCLUDE_DIR)
        message(STATUS "Overriding with custom OpenSSL headers: ${OPENSSL_INCLUDE_DIR}")
        target_include_directories(ustb-cli PRIVATE ${OPENSSL_INCLUDE_DIR})
    endif()
else()
    # 情况2: 系统 lib + 用户指定 include
    # 或 情况1 补充: OPENSSL_ROOT_DIR 已设置但 find_package 失败，手动在目录下查找
    if(OPENSSL_ROOT_DIR)
        set(_SSL_SEARCH_PATHS
            "${OPENSSL_ROOT_DIR}/lib64" "${OPENSSL_ROOT_DIR}/lib"
            "${OPENSSL_ROOT_DIR}/usr/lib64" "${OPENSSL_ROOT_DIR}/usr/lib")
        set(_INC_SEARCH "${OPENSSL_ROOT_DIR}/include" "${OPENSSL_ROOT_DIR}/usr/include")
        find_library(SSL_LIB NAMES ssl libssl PATHS ${_SSL_SEARCH_PATHS} NO_DEFAULT_PATH)
        find_library(CRYPTO_LIB NAMES crypto libcrypto PATHS ${_SSL_SEARCH_PATHS} NO_DEFAULT_PATH)
        find_path(OPENSSL_INC NAMES openssl/ssl.h PATHS ${_INC_SEARCH} NO_DEFAULT_PATH)
        if(SSL_LIB AND CRYPTO_LIB AND OPENSSL_INC)
            set(OPENSSL_OK TRUE)
            message(STATUS "OpenSSL found in OPENSSL_ROOT_DIR: ${OPENSSL_ROOT_DIR}")
            target_compile_definitions(ustb-cli PRIVATE WITH_SSL)
            target_include_directories(ustb-cli PRIVATE ${OPENSSL_INC})
            target_link_libraries(ustb-cli PRIVATE ${SSL_LIB} ${CRYPTO_LIB})
        endif()
    endif()
    if(NOT OPENSSL_OK)
        # 纯情况2: 系统 lib + 用户指定 OPENSSL_INCLUDE_DIR
        find_library(SSL_LIB NAMES ssl libssl.so.3)
        find_library(CRYPTO_LIB NAMES crypto libcrypto.so.3)
        if(SSL_LIB AND CRYPTO_LIB AND OPENSSL_INCLUDE_DIR)
            set(OPENSSL_OK TRUE)
            message(STATUS "Using system OpenSSL libs + custom headers:")
            message(STATUS "  SSL: ${SSL_LIB}")
            message(STATUS "  Crypto: ${CRYPTO_LIB}")
            message(STATUS "  Include: ${OPENSSL_INCLUDE_DIR}")
            target_compile_definitions(ustb-cli PRIVATE WITH_SSL)
            target_include_directories(ustb-cli PRIVATE ${OPENSSL_INCLUDE_DIR})
            target_link_libraries(ustb-cli PRIVATE ${SSL_LIB} ${CRYPTO_LIB})
        endif()
    endif()
    if(NOT OPENSSL_OK)
        # 情况4: 纯系统 openssl（系统 lib + 系统 include）
        find_library(SSL_LIB NAMES ssl3 libssl.so.3)
        find_library(CRYPTO_LIB NAMES crypto3 libcrypto.so.3)
        find_path(OPENSSL_INC NAMES openssl/ssl.h)
        if(SSL_LIB AND CRYPTO_LIB AND OPENSSL_INC)
            set(OPENSSL_OK TRUE)
            message(STATUS "Using system OpenSSL:")
            message(STATUS "  SSL: ${SSL_LIB}")
            message(STATUS "  Crypto: ${CRYPTO_LIB}")
            message(STATUS "  Include: ${OPENSSL_INC}")
            target_compile_definitions(ustb-cli PRIVATE WITH_SSL)
            target_include_directories(ustb-cli PRIVATE ${OPENSSL_INC})
            target_link_libraries(ustb-cli PRIVATE ${SSL_LIB} ${CRYPTO_LIB})
        endif()
    endif()
endif()

if(NOT OPENSSL_OK)
    message(WARNING "OpenSSL not found, HTTPS disabled")
endif()

#---------------------------------------
# Hash library
#---------------------------------------
# Use md5.c if no openssl, else use md5_openssl.c
if(OPENSSL_OK)
    target_sources(ustb-cli PRIVATE lib/md5_openssl.c)
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

#---------------------------------------
# Install (only ustb-cli executable)
#---------------------------------------
install(TARGETS ustb-cli
    RUNTIME DESTINATION bin
)
