#version 330 core
out vec4 FragColor;
in vec2 texCord;

in vec3 Normal;
in vec3 Position;

uniform vec3 cameraPos;
//uniform sampler2D texture0;
uniform samplerCube skybox;

void main()
{
    float ratio = 1.00 / 1.52;// 1.0为空气、1.52为玻璃的
    vec3 I = normalize(Position - cameraPos);
    //vec3 R = reflect(I, normalize(Normal)); // 实现立方体贴图的反射效果
    vec3 R = refract(I, normalize(Normal), ratio); // 实现立方体贴图的折射效果，ratio为介质之间的比值，传入/传出
    FragColor = vec4(texture(skybox, R).rgb, 1.0);
    //FragColor = texture(texture0, texCord);
}
