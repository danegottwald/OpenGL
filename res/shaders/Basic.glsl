#type vertex
#version 330 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 color;

out vec3 ourColor;

void main() {
    gl_Position = position;
    ourColor = color;
}



#type fragment
#version 330 core

layout(location = 0) out vec4 color;

in vec3 ourColor;
uniform vec4 u_Color;

void main() {
    color = vec4(ourColor, 1.0);
}

