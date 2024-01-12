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
    vec3 position;// 点光源
    vec3 lightDirect;// 定向光

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    // 光随距离衰减系数
    float constant;// 常数项一般为1
    float linear;// 线性常数
    float quadratic;// 二次项常数
};
uniform Light light;

// 聚光，像手电筒一样
struct Flashlight
{
    vec3  position;//位置
    vec3  direction;// 聚光的方向
    float cutOff;// 夹角
    float outerCutOff;//外切角，主要是平滑过渡边缘光照
};
uniform Flashlight flashlight;

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
    float distance    = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance +
                    light.quadratic * (distance * distance));

     // 环境光
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoord));

    // 漫反射
    /*
    vec3 Nor = normalize(normal);
    //vec3 lightDir = normalize(light.position - FragPos);
    vec3 lightDir = normalize(-light.lightDirect);
    float diff = max(dot(Nor, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoord));

     // 镜面反射
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, Nor);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoord));
   // FragColor = vec4((ambient + diffuse + specular) * attenuation, 1.0f);
   */

    // 聚光
    vec3 flashlightDir = normalize(flashlight.position - FragPos);
    float theta = dot(flashlightDir, normalize(-flashlight.direction));
    float epsilon   = flashlight.cutOff - flashlight.outerCutOff;
    float intensity = clamp((theta - flashlight.outerCutOff) / epsilon, 0.0, 1.0);

    if(theta > flashlight.outerCutOff)
    {
        // 漫反射
        vec3 Nor = normalize(normal);
        float diff = max(dot(Nor, normalize(-flashlight.direction)), 0.0);
        vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, TexCoord));

         // 镜面反射
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-normalize(-flashlight.direction), Nor);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoord));

        FragColor = vec4((ambient + diffuse + specular) * intensity, 1.0f);
    }
    else  // 否则，使用环境光，让场景在聚光之外时不至于完全黑暗
        FragColor = vec4(ambient, 1.0);

}



















