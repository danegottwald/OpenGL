#version 330 core

// Vertex attributes
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normals;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in uint a_textureIndex;

// Vertex outputs
out vec3 v_normal;
out vec3 v_fragPos;
out vec2 v_uv;
flat out uint v_textureIndex;

// Uniforms
uniform mat4 u_MVP;
uniform vec3 u_lightPos;

void main()
{
    vec4 worldPos = vec4(a_position, 1.0);

    v_fragPos      = worldPos.xyz;
    v_normal       = a_normals;
    v_uv           = a_uv;
    v_textureIndex = a_textureIndex;

    gl_Position = u_MVP * worldPos;
}
