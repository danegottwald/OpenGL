add_library(OpenGLCore_UI STATIC)

target_sources(OpenGLCore_UI PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/UI.cpp
    ${CMAKE_CURRENT_LIST_DIR}/UI.h
    ${CMAKE_SOURCE_DIR}/src/client/network/NetworkUI.cpp
)

target_include_directories(OpenGLCore_UI PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/include
)

target_precompile_headers(OpenGLCore_UI PRIVATE ${CMAKE_SOURCE_DIR}/src/pch_client.h)

target_link_libraries(OpenGLCore_UI PUBLIC
    imgui::imgui
    glfw
    glm::glm-header-only
)
