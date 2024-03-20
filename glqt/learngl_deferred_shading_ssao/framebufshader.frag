#version 330 core
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

const float NEAR = 0.1; // 投影矩阵的近平面
const float FAR = 100.0f; // 投影矩阵的远平面
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // 回到NDC
    return (2.0 * NEAR * FAR) / (FAR + NEAR - z * (FAR - NEAR)); // 回到线性空间
}

uniform sampler2D texture_diffuse1;
//uniform sampler2D texture_specular1;

void main()
{
    // Store the fragment position vector and depth in the first gbuffer texture
    gPosition.xyz = FragPos;
    gPosition.a = LinearizeDepth(gl_FragCoord.z);
    // Also store the per-fragment normals into the gbuffer
    gNormal = normalize(Normal);
    // And the diffuse per-fragment color
    gAlbedoSpec.rgb = texture(texture_diffuse1, TexCoords).rgb;
}
