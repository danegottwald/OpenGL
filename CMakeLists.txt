cmake_minimum_required(VERSION 3.16)
project(OpenGL)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Copy 'res' folder to output directory
add_custom_target(RES ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/res ${CMAKE_CURRENT_BINARY_DIR}/res
)

# Find required packages
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
target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE OpenGL::GL
    PRIVATE GLEW::GLEW
    PRIVATE glfw
    PRIVATE glm::glm-header-only
    PRIVATE glad::glad
    PRIVATE imgui::imgui
)
