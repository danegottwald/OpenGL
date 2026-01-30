add_library(OpenGLCore_Physics STATIC)

target_sources(OpenGLCore_Physics PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/CollisionEvents.h
    ${CMAKE_CURRENT_LIST_DIR}/EntityCollisionSystem.cpp
    ${CMAKE_CURRENT_LIST_DIR}/EntityCollisionSystem.h
)

target_include_directories(OpenGLCore_Physics PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/include
)

target_precompile_headers(OpenGLCore_Physics PRIVATE ${CMAKE_SOURCE_DIR}/src/pch_shared.h)

target_link_libraries(OpenGLCore_Physics PUBLIC
    glm::glm-header-only
)
