#ifndef PCH_H
#define PCH_H

// C++ Standard Libraries
#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>

// OpenGL Libraries
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// GLM (Math Library)
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

// ImGui (GUI Library)
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#endif // PCH_H
