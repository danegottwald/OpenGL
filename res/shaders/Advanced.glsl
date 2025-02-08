#type vertex
#version 330 core

// Vertex attributes (inputs)
layout(location = 0) in vec3 a_position;  // Vertex position (location = 0)

// Vertex output (varying)
out vec3 v_fragPos;  // Interpolated fragment position for lighting calculation

// Uniforms
uniform mat4 u_MVP;  // Model-View-Projection matrix

void main() {
    gl_Position = u_MVP * vec4(a_position, 1.0);
    v_fragPos = a_position;  // Pass the vertex position as it is
}

#type fragment
#version 330 core

// Fragment output
layout(location = 0) out vec4 frag_color;  // Final color to output (location = 0)

// Fragment input (varying)
in vec3 v_fragPos;    // Interpolated fragment position passed from vertex shader

void main() {
    // Hardcoded light position and color in camera space
    vec3 lightPos = vec3(1.0f, 2.0f, 3.0f);  // Position of the point light in camera space
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f); // White light color

    // Calculate the direction from the fragment position to the light
    vec3 lightDir = normalize(lightPos - v_fragPos);

    // Compute diffuse lighting by simulating how the surface would interact with the light
    // This is very basic since we don't have normals; it's just the inverse of the distance squared.
    float distance = length(lightPos - v_fragPos);
    float attenuation = 1.0 / (distance * distance);  // Inverse square law for light attenuation

    // Simple diffuse color based on distance and light color
    vec3 diffuse = lightColor * attenuation;

    // Fleshy color (a light peach tone)
    vec3 fleshyColor = vec3(1.0f, 0.8f, 0.6f); // Fleshy skin-like tone

    // Final color is the flesh color modulated by the diffuse lighting
    frag_color = vec4(fleshyColor * diffuse, 1.0);
}

