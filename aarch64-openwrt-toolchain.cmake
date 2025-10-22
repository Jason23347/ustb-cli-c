set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(TOOLCHAIN_DIR "$ENV{STAGING_DIR}/toolchain-aarch64_generic_gcc-13.3.0_musl")
set(TARGET_DIR "$ENV{STAGING_DIR}/target-aarch64_cortex-a53_musl")

set(CMAKE_SYSROOT "${TARGET_DIR}")
set(CMAKE_FIND_ROOT_PATH "${TARGET_DIR}")

set(CMAKE_C_COMPILER   "${TOOLCHAIN_DIR}/bin/aarch64-openwrt-linux-musl-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/bin/aarch64-openwrt-linux-musl-g++")
set(CMAKE_AR           "${TOOLCHAIN_DIR}/bin/aarch64-openwrt-linux-musl-gcc-ar")
set(CMAKE_RANLIB       "${TOOLCHAIN_DIR}/bin/aarch64-openwrt-linux-musl-gcc-ranlib")
set(CMAKE_STRIP        "${TOOLCHAIN_DIR}/bin/aarch64-openwrt-linux-musl-strip")
set(CMAKE_NM           "${TOOLCHAIN_DIR}/bin/aarch64-openwrt-linux-musl-gcc-nm")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_DIR}/bin/aarch64-openwrt-linux-musl-objcopy")
set(CMAKE_OBJDUMP      "${TOOLCHAIN_DIR}/bin/aarch64-openwrt-linux-musl-objdump")
set(CMAKE_READELF      "${TOOLCHAIN_DIR}/bin/aarch64-openwrt-linux-musl-readelf")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
