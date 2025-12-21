#version 330 core

layout(location = 0) out vec4 frag_color;

in vec3 v_normal;
in vec3 v_fragPos;
in vec2 v_uv;
flat in uint v_textureIndex;

uniform vec3           u_viewPos;
uniform sampler2DArray u_blockTextures;
uniform vec3           u_color;

void main()
{
    // Base albedo from texture array
    vec3 albedo = texture(u_blockTextures, vec3(v_uv, v_textureIndex)).rgb;

    vec3 norm = normalize(v_normal);

    vec3 lightPos         = vec3(0.0, 256.0, 0.0);
    vec3 lightColor       = vec3(1.0, 1.0, 1.0);
    float ambientStrength  = 0.15;
    float diffuseStrength  = 0.8;
    float specularStrength = 0.3;
    float shininess        = 32.0;

    vec3 lightDir = normalize(lightPos - v_fragPos);
    vec3 viewDir  = normalize(u_viewPos - v_fragPos);

    vec3 ambient = ambientStrength * lightColor;

    float diff   = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * lightColor;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec      = pow(max(dot(norm, halfwayDir), 0.0), shininess);
    vec3 specular   = specularStrength * spec * lightColor;

    vec3 lighting = ambient + diffuse + specular;

    // Apply both lighting and tint color
    vec3 color = albedo * lighting * u_color;

    frag_color = vec4(color, 1.0);
}
