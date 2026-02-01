#version 450

// ============================================================================
// Forward Pass Fragment Shader - 材质系统
// ============================================================================

// Input from Vertex Shader
layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

// Camera UBO (set 0, binding 0)
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPosition;
} camera;

// Material UBO (set 1, binding 0)
layout(set = 1, binding = 0) uniform MaterialUBO {
    vec4 baseColor;      // xyz: 颜色, w: alpha
    float metallic;
    float roughness;
    float ao;
    float padding;
} material;

// Albedo Texture (set 1, binding 1)
layout(set = 1, binding = 1) uniform sampler2D albedoTexture;

// Output
layout(location = 0) out vec4 outColor;

void main() {
    // 采样纹理并与基础颜色混合
    vec4 texColor = texture(albedoTexture, fragTexCoord);
    vec3 baseColor = texColor.rgb * material.baseColor.rgb;
    
    // 简单的方向光照明
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 normal = normalize(fragNormal);
    
    // 环境光（考虑 AO）
    vec3 ambient = vec3(0.15) * material.ao;
    
    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = vec3(0.8) * diff;
    
    // 简单的观察方向高光（粗糙度影响）
    vec3 viewDir = normalize(camera.cameraPosition.xyz - fragPosition);
    vec3 halfDir = normalize(lightDir + viewDir);
    
    // 根据粗糙度计算高光指数
    float shininess = mix(256.0, 4.0, material.roughness);
    float spec = pow(max(dot(normal, halfDir), 0.0), shininess);
    
    // 金属度影响高光颜色
    vec3 specColor = mix(vec3(0.3), baseColor, material.metallic);
    vec3 specular = specColor * spec;
    
    // 非金属保持漫反射，金属减少漫反射
    vec3 diffuseContrib = diffuse * baseColor * (1.0 - material.metallic);
    
    vec3 finalColor = ambient * baseColor + diffuseContrib + specular;
    
    outColor = vec4(finalColor, texColor.a * material.baseColor.a);
}
