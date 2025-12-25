#version 330 core

// Vertex attributes
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normals;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec3 a_tint;

// Vertex outputs
out vec3 v_normal;
out vec3 v_worldPos;
out vec2 v_uv;
out vec3 v_tint;

// Uniforms
uniform mat4 u_mvp;
uniform mat4 u_model;

void main()
{
    vec4 worldPos = u_model * vec4(a_position, 1.0);

    v_worldPos = worldPos.xyz;
    v_normal   = normalize(mat3(transpose(inverse(u_model))) * a_normals);
    v_uv       = a_uv;
    v_tint     = a_tint;

    gl_Position = u_mvp * vec4(a_position, 1.0);
}
