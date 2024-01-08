#version 330 core
out vec4 FragColor;
in vec3 normal;
in vec3 FragPos;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    float ambientIntensity = 0.2f;
    vec3 Nor = normalize(normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diffuseIntensity = max(dot(normal, lightDir), 0.0);
    float specularStrength = 0.5;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, Nor);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);// 参数是决定聚光的强度

    vec3 ambient = ambientIntensity * lightColor;
    vec3 diffuse = diffuseIntensity * lightColor;
    vec3 specular = specularStrength * spec * lightColor;
    FragColor = vec4(objectColor * (ambient + diffuse + specular), 1.0f);
}
