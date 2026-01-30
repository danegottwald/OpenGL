add_library(OpenGLCore_Network STATIC)

target_sources(OpenGLCore_Network PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/Network.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Network.h
    ${CMAKE_CURRENT_LIST_DIR}/NetworkClient.cpp
    ${CMAKE_CURRENT_LIST_DIR}/NetworkClient.h
    ${CMAKE_CURRENT_LIST_DIR}/NetworkHost.cpp
    ${CMAKE_CURRENT_LIST_DIR}/NetworkHost.h
    ${CMAKE_CURRENT_LIST_DIR}/Packet.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Packet.h
)

target_include_directories(OpenGLCore_Network PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/include
)

target_precompile_headers(OpenGLCore_Network PRIVATE ${CMAKE_SOURCE_DIR}/src/pch_shared.h)

target_link_libraries(OpenGLCore_Network PUBLIC
    glm::glm-header-only
)
