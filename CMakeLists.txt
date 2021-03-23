cmake_minimum_required(VERSION 3.16)
project(OpenGL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Renderer
set(RENDERER_DIR src/Engine/Renderer/)
set(RENDERER
        ${RENDERER_DIR}Renderer.h
        ${RENDERER_DIR}Renderer.cpp
        ${RENDERER_DIR}VertexBuffer.h
        ${RENDERER_DIR}VertexBuffer.cpp
        ${RENDERER_DIR}IndexBuffer.h
        ${RENDERER_DIR}IndexBuffer.cpp
        ${RENDERER_DIR}Shader.h
        ${RENDERER_DIR}Shader.cpp
        ${RENDERER_DIR}VertexArray.h
        ${RENDERER_DIR}VertexArray.cpp
        ${RENDERER_DIR}VertexBufferLayout.h
        ${RENDERER_DIR}VertexBufferLayout.cpp
        ${RENDERER_DIR}Texture.h
        ${RENDERER_DIR}Texture.cpp)

#Vendor
include_directories(src/vendor/)
set(VENDOR_DIR src/vendor/)
set(VENDOR
        ${VENDOR_DIR}stb_image/stb_image.h
        ${VENDOR_DIR}stb_image/stb_image.cpp)


set(SOURCES src/main.cpp)

add_executable(${PROJECT_NAME}
        ${SOURCES}
        ${RENDERER}
        ${VENDOR})

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

