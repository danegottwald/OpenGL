add_library(OpenGLCore_Client INTERFACE)

target_link_libraries(OpenGLCore_Client INTERFACE
    OpenGLCore_Common
    OpenGLCore_Renderer
    OpenGLCore_Platform
    OpenGLCore_UI
    OpenGLCore_App
)
