#version 330 core

// Vertex attributes (inputs)
layout(location = 0) in vec3 a_position;  // Vertex position (location = 0)
layout(location = 1) in vec3 a_normals;   // Vertex normal (location = 1)

// Vertex output (varying)
out vec3 v_normal;   // Interpolated normal passed to fragment shader
out vec3 v_fragPos;  // Interpolated fragment position passed to fragment shader
out vec3 v_lightDir; // Light direction passed to fragment shader

// Uniforms
uniform mat4 u_MVP;    // Model-View-Projection matrix (combined view, projection, and model)
uniform vec3 u_lightPos; // Light position in world space

void main() {
    // Vertex position remains the same, no model transformation is applied
    vec4 worldPos = vec4(a_position, 1.0); // The vertex position (model space)

    // Pass the world position to fragment shader
    v_fragPos = worldPos.xyz;

    // Apply the MVP matrix (model, view, and projection transformations)
    gl_Position = u_MVP * worldPos; // Transform to clip space

    // Pass the normal to the fragment shader (no transformation applied)
    v_normal = a_normals;

    // Calculate light direction (light position - fragment position)
    v_lightDir = normalize(u_lightPos - v_fragPos); // Light direction
}
