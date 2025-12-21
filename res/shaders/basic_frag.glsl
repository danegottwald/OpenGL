#version 330 core

// Fragment outputs
layout(location = 0) out vec4 frag_color;

// Fragment inputs
in vec3 v_normal;
in vec3 v_fragPos;
in vec2 v_uv;
flat in uint v_textureIndex;

// Uniforms
uniform vec3           u_viewPos;
uniform sampler2DArray u_blockTextures;
uniform vec3           u_color;

void main()
{
    vec3 albedo = texture(u_blockTextures, vec3(v_uv, v_textureIndex)).rgb;
    frag_color = vec4(albedo * u_color, 1.0);
}
