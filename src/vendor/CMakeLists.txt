# Special Includes for Nested Includes
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
set(GLOB_RECURSE GLAD CONFIGURE_DEPENDS "glad/*.h" "glad/*.cxx")
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include)

# Include Libraries to Maintain Folder Structure
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(GLOB_RECURSE IMGUI CONFIGURE_DEPENDS "include/imgui/*.h" "include/imgui/*.cxx")
set(GLOB_RECURSE STB_IMAGE CONFIGURE_DEPENDS "include/stb_image/*.h" "include/stb_image/*.cxx")

set(VENDOR
    ${GLAD}
    ${IMGUI}
    ${STB_IMAGE}
    PARENT_SCOPE
)

# Find and Link OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})

# Link Remaining Libraries for Windows
if (WIN32)
    # GLFW
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw)
    
    target_link_libraries(${PROJECT_NAME}
        glfw 
        ${GLFW_LIBRARIES}
        imm32.lib
    )
endif (WIN32)

# Link Remaining Libraries for Unix
if (UNIX)
    target_link_libraries(${PROJECT_NAME} 
        glfw
    )
endif (UNIX)
