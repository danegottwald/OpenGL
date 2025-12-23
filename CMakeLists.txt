cmake_minimum_required(VERSION 3.21)
project(OpenGL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5) # For vcpkg compatibility

# Find external dependencies using vcpkg
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)

# Collect source files
file(GLOB_RECURSE SRC_FILES "src/*.h" "src/*.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX ".*[/\\]pch\\.h$")

# Create executable
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    glm::glm-header-only
    glad::glad
    imgui::imgui
    JsonCpp::JsonCpp
)

# Copy resource directory
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)

# Optional: prevent console in Release
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
