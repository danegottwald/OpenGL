#version 330 core

// Fragment output
layout(location = 0) out vec4 frag_color;  // Final color to output (location = 0)

// Fragment input (varying)
in vec3 v_normal;        // Interpolated normal passed from vertex shader
in vec3 v_fragPos;       // Interpolated fragment position passed from vertex shader
in vec3 v_lightDir;      // Interpolated light direction passed from vertex shader

// Uniforms for lighting
uniform vec3 u_viewPos;    // Camera (view) position in world space
uniform vec3 u_color;      // Object color

void main() {
    // Normalize the normal vector to make sure it's a unit vector
    vec3 norm = normalize(v_normal);

    // Hardcoded lighting parameters
    float ambientStrength = 0.1;  // Ambient light strength
    float diffuseStrength = 0.8;  // Diffuse light strength
    float specularStrength = 0.5; // Specular light strength
    float shininess = 32.0;       // Shininess factor for specular highlight

    // Set the light color to white
    vec3 lightColor = vec3(1.0, 1.0, 1.0); // White light

    // Calculate ambient component: constant light for all fragments
    vec3 ambient = ambientStrength * lightColor;

    // Calculate diffuse component: based on the angle between normal and light direction
    float diff = max(dot(norm, v_lightDir), 0.0); // Clamp to zero to avoid negative values
    vec3 diffuse = diffuseStrength * diff * lightColor;

    // Calculate specular component: based on reflection of the light direction
    vec3 viewDir = normalize(u_viewPos - v_fragPos);  // Direction from the fragment to the camera
    vec3 reflectDir = reflect(-v_lightDir, norm);     // Reflection of the light direction
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);  // Specular highlight intensity
    vec3 specular = specularStrength * spec * lightColor;

    // Combine the ambient, diffuse, and specular components
    vec3 result = (ambient + diffuse + specular);

    // Multiply the result by the object's color (modulate lighting by object color)
    frag_color = vec4(result * u_color, 1.0); // Set alpha to 1.0 (opaque)
}
