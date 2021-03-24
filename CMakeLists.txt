cmake_minimum_required(VERSION 3.16)
project(OpenGL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Engine
set(ENGINE_DIR src/Engine/)
file(GLOB_RECURSE ENGINE "${ENGINE_DIR}*.h" "${ENGINE_DIR}*.cpp")

# Vendor
set(VENDOR_DIR src/vendor/)
include_directories(${VENDOR_DIR})
file(GLOB_RECURSE VENDOR "${VENDOR_DIR}*.h" "${VENDOR_DIR}*.cpp")

set(ENTRY src/main.cpp)
add_executable(${PROJECT_NAME}
        ${ENTRY}
        ${ENGINE}
        ${VENDOR}
        )

# Copy Resources Folder to Build Directory
file(COPY res DESTINATION ${CMAKE_BINARY_DIR})

# GLFW
add_subdirectory(${CMAKE_SOURCE_DIR}/Dependencies/GLFW)
include_directories(${CMAKE_SOURCE_DIR}/Dependencies/GLFW/include)
target_link_libraries(${PROJECT_NAME} glfw)

# GLEW
add_subdirectory(${CMAKE_SOURCE_DIR}/Dependencies/GLEW/build/cmake)
include_directories(${CMAKE_SOURCE_DIR}/Dependencies/GLEW/include)
target_link_libraries(${PROJECT_NAME} glew_s)

# OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} OpenGL::GL)

