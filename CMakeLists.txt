cmake_minimum_required(VERSION 3.16)
project(OpenGL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set Output Directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Engine
set(ENGINE_DIR src/Engine/)
file(GLOB_RECURSE ENGINE "${ENGINE_DIR}*.h" "${ENGINE_DIR}*.cpp")

# Process Vendor Libraries
add_subdirectory(src/vendor)

set(ENTRY src/Main.cpp)
add_executable(${PROJECT_NAME}
    ${ENTRY}
    ${ENGINE}
    ${VENDOR}
    ${TESTS}
)

# Vendor Include Paths
include_directories(${VENDOR_INCLUDES})

# Link Libraries
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} 
    ${OPENGL_LIBRARIES}
    ${VENDOR_LIBRARIES}
)

