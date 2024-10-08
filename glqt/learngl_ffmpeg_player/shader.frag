#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D image;

void main()
{
   color = vec4(texture(image, TexCoords).rgb, 1.0);
}
