#ifndef PCH_H
#define PCH_H

// C++ Standard Libraries
#include <algorithm>
#include <array>
#include <cassert>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <expected>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <print>
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <span>
#include <sstream>
#include <stack>
#include <string>
#include <tuple>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Additional Libraries
#include <sal.h>

// OpenGL Libraries
#include <glad/glad.h>
#include <gl/glext.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

// GLM (Math Library)
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/vec3.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

// ImGui (GUI Library)
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// Macro Definitions
class ScopeTimer
{
public:
   ScopeTimer( std::string_view label ) :
      m_label( label ),
      m_start( std::chrono::high_resolution_clock::now() )
   {}

   ~ScopeTimer()
   {
      double ms = std::chrono::duration< double, std::milli >( std::chrono::high_resolution_clock::now() - m_start ).count();
      std::println( "{} took {:.3f}ms", m_label, ms );
   }

private:
   std::string                                    m_label;
   std::chrono::high_resolution_clock::time_point m_start;
};
#define PROFILE_SCOPE( name ) ScopeTimer timer##__LINE__{name}

#endif // PCH_H
