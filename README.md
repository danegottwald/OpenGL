# OpenGL Playground – Graphics, Game Programming, and Networking

This repository is a personal learning project focused on exploring and implementing core concepts in **OpenGL**, modern **game programming design patterns**, and **networking (multiplayer)**. The codebase is structured to help me (and others) understand how real-time rendering, game architecture, and networked multiplayer systems work together in a C++ environment.

## Project Goals

- **Learn and apply OpenGL fundamentals**: Rendering, shaders, camera systems, and 3D math.
- **Experiment with game programming patterns**: Entity-Component-System (ECS), event-driven architecture, and modular design.
- **Implement multiplayer networking**: Client-server model, packet serialization, and real-time data synchronization.

## Features

- **OpenGL Rendering**: Custom rendering pipeline using modern OpenGL (GLFW, glad, GLM).
- **Game Loop & Architecture**: Modular structure with clear separation of concerns (core, entities, world, input, GUI).
- **Entity System**: Basic player and world entities, extensible for more game objects.
- **Event System**: Decoupled event handling for game and network events.
- **Multiplayer Networking**: 
  - Client and host/server implementations.
  - Packet-based communication and serialization.
  - Real-time position updates and chat messages.
- **ImGui Integration**: In-game GUI for debugging and controls.

## Libraries Used

- [GLFW](https://www.glfw.org/) – Window/context/input management
- [glad](https://github.com/Dav1dde/glad/tree/glad2) – OpenGL function loader
- [GLM](https://github.com/g-truc/glm) – Math library for graphics
- [STB](https://github.com/nothings/stb/blob/master/stb_image.h) – Image loading
- [ImGui](https://github.com/ocornut/imgui) – Immediate mode GUI
- [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) - Noise generation

## Learning Resources

- [docs.GL](http://docs.gl/#)
- [LearnOpenGL](https://learnopengl.com/Introduction)
- [Learning Modern 3D Graphics Programming](https://nicolbolas.github.io/oldtut/)
