#version 330 core

// Fragment outputs
layout(location = 0) out vec4 frag_color;

// Fragment inputs
in vec3 v_normal;
in vec3 v_worldPos;
in vec2 v_uv;
in vec3 v_tint;

uniform vec3      u_viewPos;
uniform sampler2D u_blockTextures;

// Sun (directional light). Direction points *from fragment toward light*.
uniform vec3 u_sunDirection;
uniform vec3 u_sunColor;
uniform vec3 u_ambientColor;

void main()
{
    vec3 albedo = texture(u_blockTextures, v_uv).rgb * v_tint;

    vec3 N = normalize(v_normal);
    vec3 V = normalize(u_viewPos - v_worldPos);
    vec3 L = normalize(u_sunDirection);

    vec3 ambient = u_ambientColor * albedo;

    float NdotL = max(dot(N, L), 0.0);

    float wrap = 0.25;
    float wrapped = clamp((NdotL + wrap) / (1.0 + wrap), 0.0, 1.0);
    vec3 diffuse = wrapped * u_sunColor * albedo;

    vec3  H = normalize(L + V);
    float specPower = 64.0;
    float specStrength = 0.18;
    float spec = pow(max(dot(N, H), 0.0), specPower) * specStrength;

    vec3 specular = spec * mix(u_sunColor, vec3(0.65, 0.75, 1.0), 0.35);

    vec3 color = ambient + diffuse + specular;
    color = color / (color + vec3(1.0));

    frag_color = vec4(color, 1.0);
}
