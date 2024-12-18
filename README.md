# **OpenGL**

This repository is my OpenGL playground to learn and expand my knownledge of computer graphics and rendering.
Since this is mostly a learning experience for me, I will share the resources I have found and have mostly been following:

- [The Cherno](https://www.youtube.com/watch?v=W3gAzLwfIP0&list=PLlrATfBNZ98foTJPJ_Ev03o2oq3-GGOS2) : OpenGL Tutorial Series
- [docs.GL](http://docs.gl/#) : OpenGL API Documentation
- [LearnOpenGL](https://learnopengl.com/Introduction) : Written Guide to OpenGL
- [Learning Modern 3D Graphics Programming](https://nicolbolas.github.io/oldtut/) : Written Guide to OpenGL/Modern Graphics Programming

<!-- LIBRARIES -->
## Libraries

I am currently using the following libraries:

- [GLFW](https://www.glfw.org/) : OpenGL Utility Library
- [glad](https://github.com/Dav1dde/glad/tree/glad2) : OpenGL Extension Library
- [GLM](https://github.com/g-truc/glm) : OpenGL Math Library
- [STB](https://github.com/nothings/stb/blob/master/stb_image.h) : Image Loader
- [ImGui](https://github.com/ocornut/imgui) : GUI Library

<!-- Getting Started -->
## Getting Started

If the repository was cloned non-recursively previously, use git submodule update --init to clone the necessary submodules.

### Prerequisites

### Installation

```bash
# Clone repository
git clone --recursive https://github.com/danegottwald/OpenGL.git

# Go to project directory
cd OpenGL

# Run cmake
cmake -G "Unix Makefiles" -B build

# Go to build directory
cd build

# Run make
make
```
