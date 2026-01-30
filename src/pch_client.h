#ifndef PCH_CLIENT_H
#define PCH_CLIENT_H

#include "pch_shared.h"

// Client-only dependencies (windowing / rendering / UI)
#include <glad/glad.h>
#include <gl/glext.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#endif // PCH_CLIENT_H
