cmake_minimum_required(VERSION 3.18)
project(NVI-PFA LANGUAGES C CXX)

# Specify the Android toolchain
set(CMAKE_TOOLCHAIN_FILE "$ENV{ANDROID_NDK}/build/cmake/android.toolchain.cmake")

# Target ABI and platform
set(ANDROID_ABI arm64-v8a)
set(ANDROID_PLATFORM android-33)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Include SDL3
set(SDL3_DIR "${CMAKE_SOURCE_DIR}/SDL")  # path to SDL3 folder
find_package(SDL3 REQUIRED)

# Add your source files
add_library(NVPFA SHARED
    src/main/cpp/NVPFAActivity.cpp
    src/main/cpp/other_source.cpp
)

# Link SDL3
target_link_libraries(NVPFA PRIVATE SDL3::SDL3)

# Specify output folder for .so files
set_target_properties(NVPFA PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/libs/${ANDROID_ABI}"
)
