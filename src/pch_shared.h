#ifndef PCH_SHARED_H
#define PCH_SHARED_H

// Prevent Windows SDK headers from defining macros that conflict with our identifiers.
#ifndef NOMINMAX
#define NOMINMAX
#endif

// C++ Standard Libraries
#include <algorithm>
#include <any>
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
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
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
#include <ranges>
#include <regex>
#include <set>
#include <shared_mutex>
#include <span>
#include <sstream>
#include <stack>
#include <stdexcept>
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

#ifndef OPENGL_COMMON_PCH_UTILS
#define OPENGL_COMMON_PCH_UTILS

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

// Class macros
#define NO_COPY( ClassName )                 \
    ClassName(const ClassName&) = delete;    \
    ClassName& operator=(const ClassName&) = delete;
#define NO_MOVE( ClassName )                 \
    ClassName(ClassName&&) = delete;         \
    ClassName& operator=(ClassName&&) = delete;
#define NO_COPY_MOVE( ClassName ) NO_COPY( ClassName ) NO_MOVE( ClassName )

#endif // OPENGL_COMMON_PCH_UTILS

#endif // PCH_SHARED_H
