#version 330 core
layout (location = 0) in vec3 aPos;
//layout (location = 1) in vec2 aTexCord;
layout (location = 1) in vec3 aNormal;
//out vec2 texCord;
out vec3 Normal;
out vec3 Position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    Normal = mat3(transpose(inverse(model))) * aNormal;// 去掉平移效果
    Position = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    //texCord = aTexCord;
}
