add_library(OpenGLCore_Renderer STATIC)

target_sources(OpenGLCore_Renderer PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/IndexBuffer.h
    ${CMAKE_CURRENT_LIST_DIR}/Mesh.h
    ${CMAKE_CURRENT_LIST_DIR}/Shader.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Shader.h
    ${CMAKE_CURRENT_LIST_DIR}/Texture.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Texture.h
    ${CMAKE_CURRENT_LIST_DIR}/VertexArray.h
    ${CMAKE_CURRENT_LIST_DIR}/VertexBuffer.h
    ${CMAKE_CURRENT_LIST_DIR}/VertexBufferLayout.cpp
    ${CMAKE_CURRENT_LIST_DIR}/VertexBufferLayout.cpp
    ${CMAKE_CURRENT_LIST_DIR}/VertexBufferLayout.h
)

target_include_directories(OpenGLCore_Renderer PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/include
)

target_precompile_headers(OpenGLCore_Renderer PRIVATE ${CMAKE_SOURCE_DIR}/src/pch_shared.h)

find_package(Stb REQUIRED)
target_include_directories(OpenGLCore_Renderer PUBLIC ${Stb_INCLUDE_DIR})

target_link_libraries(OpenGLCore_Renderer PUBLIC
    OpenGL::GL
    glad::glad
    glm::glm-header-only
)
