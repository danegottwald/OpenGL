cmake_minimum_required(VERSION 3.21)
project(OpenGL)

# Set options
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /ENTRY:mainCRTStartup")

# Find external dependencies using vcpkg
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Stb REQUIRED)

# Collect all source files
file(GLOB_RECURSE SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "src/*.h" "src/*.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX ".*[/\\]pch\\.h$")

# Create the executable and link necessary libraries
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

# Copy resource directory to build directory
set(RES_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/res")
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${RES_DIRECTORY} "${CMAKE_CURRENT_BINARY_DIR}/res")

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
   ${CMAKE_SOURCE_DIR}/src
   ${CMAKE_SOURCE_DIR}/include
   ${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include
)

# Link external libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
   OpenGL::GL
   glfw
   glm::glm-header-only
   glad::glad
   imgui::imgui
)

# Prevent console window from showing on Release builds
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)

# Install the executable and copy dependencies
install(DIRECTORY ${RES_DIRECTORY} DESTINATION ${CMAKE_INSTALL_PREFIX})
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> DESTINATION ${CMAKE_INSTALL_PREFIX} OPTIONAL)

# Policies
cmake_policy(SET CMP0087 NEW)
