cmake_minimum_required(VERSION 3.21)
project(OpenGL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5) # For vcpkg compatibility

# Find external dependencies (usually via vcpkg)
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)

set(CLIENT_MAIN "src/client/ClientMain.cpp")
set(SERVER_MAIN "src/server/ServerMain.cpp")

# Modules
add_subdirectory(src/shared)
add_subdirectory(src/client)
add_subdirectory(src/server)

# Application layer (client-only for now; depends on Window/UI)
add_library(OpenGLCore_App STATIC
    src/shared/Engine/Core/Application.cpp
    src/shared/Engine/Core/Application.h
    src/shared/Engine/Core/GameStateStack.cpp
    src/shared/Engine/Core/GameStateStack.h
)
target_precompile_headers(OpenGLCore_App PRIVATE src/pch_client.h)
target_include_directories(OpenGLCore_App PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(OpenGLCore_App PUBLIC
    OpenGLCore_Common
    OpenGLCore_Platform
    OpenGLCore_UI
)

# --- Executables ---
add_executable(${PROJECT_NAME} ${CLIENT_MAIN})
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGLCore_Client)
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch_client.h)

add_executable(${PROJECT_NAME}_Server ${SERVER_MAIN})
target_link_libraries(${PROJECT_NAME}_Server PRIVATE OpenGLCore_Server)
target_precompile_headers(${PROJECT_NAME}_Server PRIVATE src/pch_server.h)

# Client-only asset staging
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)

# For Release (client)
target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>)
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)

add_custom_target(Default ALL DEPENDS ${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_Server RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> DESTINATION ${CMAKE_INSTALL_PREFIX} OPTIONAL)
cmake_policy(SET CMP0087 NEW)
