add_library(OpenGLCore_World STATIC)

target_sources(OpenGLCore_World PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/Blocks.h
    ${CMAKE_CURRENT_LIST_DIR}/BlockDefs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/BlockDefs.h
    ${CMAKE_CURRENT_LIST_DIR}/ChunkRenderer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ChunkRenderer.h
    ${CMAKE_CURRENT_LIST_DIR}/Level.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Level.h
    ${CMAKE_CURRENT_LIST_DIR}/Raycast.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Raycast.h
    ${CMAKE_CURRENT_LIST_DIR}/RenderSystem.cpp
    ${CMAKE_CURRENT_LIST_DIR}/RenderSystem.h
    ${CMAKE_CURRENT_LIST_DIR}/WorldSave.cpp
    ${CMAKE_CURRENT_LIST_DIR}/WorldSave.h
)

target_include_directories(OpenGLCore_World PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/include
)

target_precompile_headers(OpenGLCore_World PRIVATE ${CMAKE_SOURCE_DIR}/src/pch_shared.h)

target_link_libraries(OpenGLCore_World PUBLIC
    OpenGLCore_ECS
    glm::glm-header-only
    JsonCpp::JsonCpp
)
