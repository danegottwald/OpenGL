add_library(OpenGLCore_Game STATIC)

target_sources(OpenGLCore_Game PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/InGameState.cpp
    ${CMAKE_CURRENT_LIST_DIR}/InGameState.h
    ${CMAKE_CURRENT_LIST_DIR}/MainMenuState.cpp
    ${CMAKE_CURRENT_LIST_DIR}/MainMenuState.h
)

target_include_directories(OpenGLCore_Game PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/include
)

target_precompile_headers(OpenGLCore_Game PRIVATE ${CMAKE_SOURCE_DIR}/src/pch_shared.h)

target_link_libraries(OpenGLCore_Game PUBLIC
    OpenGLCore_ECS
    OpenGLCore_World
    OpenGLCore_Network
    OpenGLCore_Physics
    OpenGLCore_Platform
    OpenGLCore_Renderer
    OpenGLCore_UI
    glm::glm-header-only
    JsonCpp::JsonCpp
)
