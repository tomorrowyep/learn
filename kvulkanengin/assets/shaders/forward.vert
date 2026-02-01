#version 450

// ============================================================================
// Forward Pass Vertex Shader
// ============================================================================

// Vertex Input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

// Camera UBO (set 0, binding 0)
layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPosition;
} camera;

// Push Constant: Model Matrix
layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

// Output to Fragment Shader
layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    
    fragPosition = worldPos.xyz;
    fragNormal = mat3(transpose(inverse(push.model))) * inNormal;
    fragTexCoord = inTexCoord;
    
    gl_Position = camera.viewProjection * worldPos;
}
