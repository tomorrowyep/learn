#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

uniform vec3 camPos;

uniform vec3  albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

// IBL
uniform samplerCube irradianceMap;

uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

const float PI = 3.14159265359;

// 获取与半程向量方向一致的比例，返回值（在 [0, +∞) ）越大表示微表面中与半程向量方向一致的微平面法线比例越高，即在该方向上有更多的反射光线，因此表面看起来更光滑
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;// epic在实践中采用粗糙度的平方效果会更好
    float a2 = a * a;

    float ndotH = max(dot(N, H), 0.0);
    float ndotH2 = ndotH * ndotH;

    float denom = ndotH2 *(a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return a2 / denom;
}

// Fresnel函数（菲涅尔方程），获取反射占比KS，返回值是一个表示反射光强度的贡献值，范围在 [0, 1] 之间
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float k = pow((roughness + 1.0), 2) / 8.0;
    float denom = NdotV * (1.0 - k) + k;

    return NdotV / denom;
}

// 计算遮蔽的比例，范围在 [0, 1] 之间
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float nDotV = max(dot(N, V), 0.0);
    float nDotL = max(dot(N, L), 0.0);
    float ggx1  = GeometrySchlickGGX(nDotV, roughness);
    float ggx2  = GeometrySchlickGGX(nDotL, roughness);

    return ggx1 * ggx2;
}

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);

    vec3 F0 = vec3(0.04);// PBR金属流中我们简单地认为大多数的绝缘体在F0为0.04的时候看起来视觉上是正确的
    F0 = mix(F0, albedo, metallic);// 线性插值，金属度为0则为F0，为1则为albedo，返回值为（1-metallic）*F0 + metallic * albedo

    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; ++i)
    {
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);

        // 计算入射光线的辐射率
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;// 光线到达这个点衰减了多少

        // 法线分布函数，返回值越大表示反射光占比越大，表面就越光滑
        float NDF = DistributionGGX(N, H, roughness);

        // 返回反射光强度的贡献值KS
        vec3 F =  FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

        // 返回当前像素被周围几何体的遮蔽程度，越接近 1 表示像素受到的遮蔽越多，表面会显得更暗
        float G = GeometrySmith(N, V, L, roughness);

        vec3 nominator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;// 防止除数为零
        vec3 specular = nominator / denominator;

        vec3 kS = F;// 菲涅尔方程的返回值就是反射光线的贡献值（0-1）
        vec3 kD = vec3(1.0) - kS;

        kD *= (1.0 - metallic);// 根据金属材质（金属材质没有折射光线，全部被吸收）占比来获取相对正确的参数
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;// 离散的渲染方程
    }

    // ambient lighting (we now use IBL as the ambient term)
    vec3 kS = FresnelSchlick(max(dot(N, V), 0.0), F0);
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo;
    vec3 ambient = (kD * diffuse) * ao;
    vec3 color  = ambient + Lo;

    // 色调映射以及伽马校正（在线性空间）
    color = color / (color + vec3(1.0));// Reinhard映射 从HDR映射到LDR（保持颜色的正确性）
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}

