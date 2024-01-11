#version 330 core
out vec4 FragColor;
in vec3 normal;
in vec3 FragPos;
in vec2 TexCoord;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 viewPos;

// 材质
/*struct Material
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};*/
struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float     shininess;
};
uniform Material material;

// 光照分量
struct Light
{
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Light light;

void main()
{
    /*
    float ambientIntensity = 0.2f;
    vec3 Nor = normalize(normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diffuseIntensity = max(dot(Nor, lightDir), 0.0);
    float specularStrength = 0.5;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, Nor);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);// 参数是决定聚光的强度

    vec3 ambient = ambientIntensity * lightColor;
    vec3 diffuse = diffuseIntensity * lightColor;
    vec3 specular = specularStrength * spec * lightColor;
    FragColor = vec4(objectColor * (ambient + diffuse + specular), 1.0f);
    */

    /* 材质的相关代码
     // 环境光
     vec3 ambient = light.ambient * material.ambient;

     // 漫反射
     vec3 Nor = normalize(normal);
     vec3 lightDir = normalize(light.position - FragPos);
     float diff = max(dot(Nor, lightDir), 0.0);
     vec3 diffuse = light.diffuse * (diff * material.diffuse);

     // 镜面反射
     vec3 viewDir = normalize(viewPos - FragPos);
     vec3 reflectDir = reflect(-lightDir, Nor);
     float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
     vec3 specular = light.specular * (spec * material.specular);
     */

    // 添加了反射光的贴图的代码
     // 环境光
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoord));

    // 漫反射
    vec3 Nor = normalize(normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(Nor, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoord));

     // 镜面反射
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, Nor);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoord));
    FragColor = vec4((ambient + diffuse + specular), 1.0f);
}



















