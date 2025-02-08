cmake_minimum_required(VERSION 3.21)
project(OpenGL)

# Set C++ standard to C++23 and apply necessary compiler flags for Release builds
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG /Zi")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")

# Define a list of resource dependency directories to copy
set(RES_DIRECTORIES
    "${CMAKE_CURRENT_SOURCE_DIR}/res"
    # Add more directories as needed
)

# Copy all resource dependency directories in the list to the build directory
add_custom_target(res ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RES_DIRECTORIES} ${CMAKE_CURRENT_BINARY_DIR}/res
)

# Find external dependencies using vcpkg
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Stb REQUIRED)

# Collect all source files
file(GLOB_RECURSE FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# Create the executable and link it with necessary libraries
add_executable(${PROJECT_NAME} ${FILES})
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include)
target_link_libraries(${PROJECT_NAME}
    PRIVATE OpenGL::GL
    PRIVATE GLEW::GLEW
    PRIVATE glfw
    PRIVATE glm::glm-header-only
    PRIVATE glad::glad
    PRIVATE imgui::imgui
)

# Install the executable, copy resource dependency directories, and DLLs into the install directory
foreach(dir IN LISTS RES_DIRECTORIES)
    install(DIRECTORY ${dir} DESTINATION ${CMAKE_INSTALL_PREFIX})
endforeach()
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/res DESTINATION ${CMAKE_INSTALL_PREFIX})
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> DESTINATION ${CMAKE_INSTALL_PREFIX} OPTIONAL)

# Ensure runtime DLLs are resolved at install time
cmake_policy(SET CMP0087 NEW)
