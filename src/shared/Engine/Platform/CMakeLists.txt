add_library(OpenGLCore_Platform STATIC)

target_sources(OpenGLCore_Platform PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/RenderContext.cpp
    ${CMAKE_CURRENT_LIST_DIR}/RenderContext.h
    ${CMAKE_CURRENT_LIST_DIR}/Window.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Window.h
)

target_include_directories(OpenGLCore_Platform PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/include
)

target_precompile_headers(OpenGLCore_Platform PRIVATE ${CMAKE_SOURCE_DIR}/src/pch_client.h)

target_link_libraries(OpenGLCore_Platform PUBLIC
    glfw
    OpenGL::GL
    glad::glad
    glm::glm-header-only
)
