# Special Includes for Nested Includes
set(GLAD_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
set(GLAD 
    "${CMAKE_CURRENT_SOURCE_DIR}/glad/include/glad/gl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/glad/include/KHR/khrplatform.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/glad/src/gl.c"
)

set(GLFW_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include)
if (WIN32)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/glfw)
endif (WIN32)

# Include Libraries to Maintain Folder Structure
file(GLOB_RECURSE IMGUI CONFIGURE_DEPENDS 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/imgui/*.cpp"
)
set(STB_IMAGE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/stb_image/stb_image.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/stb_image/stb_image.cpp"
) 

# Vendor Source Files
set(VENDOR
    ${GLAD}
    ${IMGUI}
    ${STB_IMAGE}
    PARENT_SCOPE
)

# Vendor Include Paths
set(VENDOR_INCLUDES
    ${GLAD_INCLUDE}
    ${GLFW_INCLUDE}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    PARENT_SCOPE
)

# Libraries for Windows
if (WIN32)
    set(VENDOR_LIBRARIES
        glfw
        ${GLFW_LIBRARIES}
        imm32.lib
        PARENT_SCOPE
    )
endif (WIN32)

# Libraries for Unix-like
if (UNIX)
    set(VENDOR_LIBRARIES
        glfw
        PARENT_SCOPE
    )
endif (UNIX)

