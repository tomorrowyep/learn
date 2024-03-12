#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];
uniform sampler2D diffuseTexture;

void main()
{
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    // ambient
    vec3 ambient = 0.0 * color;
    // lighting
    vec3 lighting = vec3(0.0);
    for(int i = 0; i < 4; i++)
    {
        // diffuse
        vec3 lightDir = normalize(lightPositions[i] - fs_in.FragPos);
        float diff = max(dot(lightDir, normal), 0.0);
        vec3 diffuse = lightColors[i] * diff * color;
        vec3 result = diffuse;
        // attenuation (use quadratic as we have gamma correction)
        float distance = length(fs_in.FragPos - lightPositions[i]);
        result *= 1.0 / (distance * distance);
        lighting += result;
    }
    FragColor = vec4(ambient + lighting, 1.0);
}
