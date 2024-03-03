#version 330 core
out vec4 FragColor;
in vec2 texCord;

uniform sampler2D texture0;

void main()
{
    //FragColor = texture(texture0, texCord);
    FragColor = vec4(0.04, 0.28, 0.26, 1.0);
}
