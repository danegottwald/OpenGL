add_library(OpenGLCore_ServerRuntime STATIC)

target_sources(OpenGLCore_ServerRuntime PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/ServerApp.cpp
)

target_include_directories(OpenGLCore_ServerRuntime PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/include
)

target_precompile_headers(OpenGLCore_ServerRuntime PRIVATE ${CMAKE_SOURCE_DIR}/src/pch_shared.h)

target_link_libraries(OpenGLCore_ServerRuntime PUBLIC
    OpenGLCore_World
    OpenGLCore_Network
    OpenGLCore_ECS
)

add_library(OpenGLCore_Server INTERFACE)

target_link_libraries(OpenGLCore_Server INTERFACE
    OpenGLCore_Common
    OpenGLCore_ServerRuntime
)
