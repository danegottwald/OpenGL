add_library(OpenGLCore_Common INTERFACE)

target_include_directories(OpenGLCore_Common INTERFACE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(OpenGLCore_Common INTERFACE
    OpenGLCore_ECS
    OpenGLCore_Network
    OpenGLCore_Physics
    OpenGLCore_World
    OpenGLCore_Game
    glm::glm-header-only
    JsonCpp::JsonCpp
)
