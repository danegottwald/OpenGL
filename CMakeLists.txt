cmake_minimum_required(VERSION 3.17)
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

# Tests
set(TESTS_DIR src/tests/)
#file(GLOB_RECURSE TESTS "${TESTS_DIR}*.h" "${TESTS_DIR}*.cpp")

# Set Output Directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(ENTRY src/Main.cpp)
add_executable(${PROJECT_NAME}
        ${ENTRY}
        ${ENGINE}
        ${VENDOR}
        ${TESTS}
)

# Set Include Directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Link Libraries
set(LIB_DIR ${CMAKE_SOURCE_DIR}/lib)

find_package(OpenGL REQUIRED)

target_link_libraries(${PROJECT_NAME} 
        ${LIB_DIR}/glew32.lib
        ${LIB_DIR}/libglfw3dll.a
        imm32.lib
        ${OPENGL_LIBRARIES}
)

