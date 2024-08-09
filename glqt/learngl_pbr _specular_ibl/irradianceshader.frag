#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main()
{
    vec3 N = normalize(WorldPos);

    vec3 irradiance = vec3(0.0);

    // 对环境贴图卷积，求的某点的辐照度值（对半球积分）
    vec3 up = vec3(0.0f, 1.0f, 0.0f);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    float sampleDeta = 0.025; // 步数越低精度越高
    int sampleNum = 0;

    for (float phi = 0.0; phi < 2 * PI; phi += sampleDeta)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDeta)
        {
            vec3 tnbCoord = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 worldCoord = tnbCoord.x * right + tnbCoord.y * up + tnbCoord.z * N;

            irradiance += texture(environmentMap, worldCoord).rgb * cos(theta) * sin(theta);
            sampleNum++;
        }
    }

    irradiance = PI * irradiance * (1.0 / sampleNum);
    FragColor = vec4(irradiance, 1.0);
}
