#version 330 core
out vec4 FragColor;
in vec2 texCord;

uniform sampler2D textureWall;
uniform sampler2D textureFace;

void main()
{
    FragColor = mix(texture(textureWall, texCord),
                    texture(textureFace, texCord), 0.5);
    //FragColor = texture(textureWall, texCord);
}
