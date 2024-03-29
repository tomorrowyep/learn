#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform samplerCube  depthMap;
uniform sampler2D diffuseTexture;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform float far_plane;

float ShadowCalculation(vec3 fragPos)
{
    // Get vector between fragment position and light position
      vec3 fragToLight = fragPos - lightPos;
      // Use the light to fragment vector to sample from the depth map
      float closestDepth = texture(depthMap, fragToLight).r;
      // It is currently in linear range between [0,1]. Re-transform back to original value
      closestDepth *= far_plane;
      // Now get current linear depth as the length between the fragment and light position
      float currentDepth = length(fragToLight);
      // Now test for shadows
      float bias = 0.05;
      float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;

      return shadow;
}

void main()
{
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightColor = vec3(1.0);
    // Ambient
    vec3 ambient = 0.3 * color;
    // Diffuse
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // Specular
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;
    // Calculate shadow
    float shadow = ShadowCalculation(fs_in.FragPos);
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;

    FragColor = vec4(lighting, 1.0f);

    // 显示阴影
    /*vec3 fragToLight = fs_in.FragPos - lightPos;
    float closestDepth = texture(depthMap, fragToLight).r;
    closestDepth *= far_plane;
    FragColor = vec4(vec3(closestDepth / far_plane), 1.0);*/
}
