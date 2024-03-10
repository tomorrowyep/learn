#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texCoords;

out vec2 TexCoords;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f);
    vs_out.FragPos = vec3(model * vec4(position, 1.0)); // 获取到在世界坐标中的位置，因为光照都是在世界坐标下进行的
    vs_out.Normal = transpose(inverse(mat3(model))) * normal;// 将法向量从模型空间正确转换到世界坐标
    vs_out.TexCoords = texCoords;
}
