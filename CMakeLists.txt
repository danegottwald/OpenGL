cmake_minimum_required(VERSION 3.16)
project(OpenGL)

# Use vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
message(STATUS "Using vcpkg from: $ENV{VCPKG_ROOT}")

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable optimization flags for Release builds
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG")

# Copy 'res' folder to output directory
add_custom_target(RES ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/res ${CMAKE_CURRENT_BINARY_DIR}/res
)

# Find required packages using vcpkg
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Stb REQUIRED)

# Source files
file(GLOB_RECURSE FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Add the executable
add_executable(${PROJECT_NAME} ${FILES})

# Precompiled header
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

# Add dependencies for the 'res' folder
add_dependencies(${PROJECT_NAME} RES)

# Include directories for external libraries
target_include_directories(${PROJECT_NAME} PRIVATE ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE OpenGL::GL
    PRIVATE GLEW::GLEW
    PRIVATE glfw
    PRIVATE glm::glm-header-only
    PRIVATE glad::glad
    PRIVATE imgui::imgui
)

