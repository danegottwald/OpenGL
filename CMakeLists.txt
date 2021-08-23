cmake_minimum_required(VERSION 3.17)
project(OpenGL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Engine
set(ENGINE_DIR src/Engine/)
file(GLOB_RECURSE ENGINE "${ENGINE_DIR}*.h" "${ENGINE_DIR}*.cpp")

# Set Output Directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(ENTRY src/Main.cpp)
add_executable(${PROJECT_NAME}
    ${ENTRY}
    ${ENGINE}
    ${VENDOR}
    ${TESTS}
)

# Include and Link Vendor Libraries
add_subdirectory(src/vendor)

# Find and Link OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})

# Link Remaining Libraries for Windows
if (WIN32)
    set(LIB_DIR ${CMAKE_SOURCE_DIR}/lib)
    target_link_libraries(${PROJECT_NAME} 
        ${LIB_DIR}/glew32.lib
        ${LIB_DIR}/libglfw3dll.a
        imm32.lib
    )
endif (WIN32)

# Link Remaining Libraries for Unix
if (UNIX)
    target_link_libraries(${PROJECT_NAME} 
        libGLEW.so
        glfw
    )
endif (UNIX)
