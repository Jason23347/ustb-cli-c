cmake_minimum_required(VERSION 3.16)
project(UnitTests C)

enable_testing()

set(CMAKE_C_FLAGS "-g -O0")

# CMocka
include(FetchContent)

# Disable cmocka examples and internal tests
set(WITH_EXAMPLES OFF CACHE BOOL "Disable CMocka examples" FORCE)
set(WITH_TESTS OFF CACHE BOOL "Disable CMocka self-tests" FORCE)

FetchContent_Declare(
    cmocka
    URL https://cmocka.org/files/1.1/cmocka-1.1.8.tar.xz
)
FetchContent_MakeAvailable(cmocka)

set(WRAP_OPTS
    -Wl,--wrap=_test_malloc
    -Wl,--wrap=_test_calloc
    -Wl,--wrap=_test_realloc
    -Wl,--wrap=tcp_connect
    -Wl,--wrap=tcp_read
    -Wl,--wrap=tcp_write
    -Wl,--wrap=gbuff_init
    -Wl,--wrap=gbuff_ensure
    -Wl,--wrap=gbuff_realloc
    -Wl,--wrap=gbuff_put
)

# ===========================================
# libmock
# ===========================================
add_library(mock STATIC
    mock/alloc.c
    mock/gbuff.c
    mock/mem.c
    mock/tcp.c

    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/gbuff.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/gstr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calc/fee.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calc/flow.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calc/timer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/cookiejar.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/net/http.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/text.c
)
target_include_directories(mock PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_link_libraries(mock PRIVATE cmocka::cmocka)
target_compile_options(mock PRIVATE --coverage)
target_compile_definitions(mock PRIVATE main=mock_text_main)

# ===========================================
# test formats
# ===========================================
add_executable(test_formats
    test_formats.c
)
target_link_libraries(test_formats PRIVATE mock cmocka::cmocka)
target_include_directories(test_formats PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_compile_definitions(test_formats PRIVATE TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

add_test(NAME test_formats COMMAND test_formats)

target_compile_options(test_formats PRIVATE --coverage)
target_link_options(test_formats PRIVATE --coverage ${WRAP_OPTS})

# ===========================================
# test http
# ===========================================
add_executable(test_http
    test_http.c
)
target_link_libraries(test_http PRIVATE mock cmocka::cmocka)
target_include_directories(test_http PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_compile_definitions(test_http PRIVATE TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

add_test(
    NAME test_http
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/test_runner.sh
            $<TARGET_FILE:test_http>
)

target_compile_options(test_http PRIVATE --coverage)
target_link_options(test_http PRIVATE --coverage ${WRAP_OPTS})

# ===========================================
# test commands
# ===========================================
add_executable(test_commands
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/cmd/balance.c
    redirect/redirect_stdout.c
    test_commands.c
)
target_link_libraries(test_commands PRIVATE mock cmocka::cmocka)
target_include_directories(test_commands PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_compile_definitions(test_commands PRIVATE TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

add_test(NAME test_commands COMMAND test_commands)
target_compile_options(test_commands PRIVATE --coverage)
target_link_options(test_commands PRIVATE --coverage ${WRAP_OPTS})

# ===========================================
#  Coverage target
# ===========================================
add_custom_target(coverage
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND ${CMAKE_COMMAND} -E echo "Generating coverage report..."
    COMMAND gcovr
        -r ${CMAKE_CURRENT_SOURCE_DIR}/..
        --object-directory=.
        --filter='.*/src/.*'
        --filter='.*/lib/.*'
        --html-theme github.green
        --html
        --html-details
        -o coverage.html
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests and generating coverage report"
)
