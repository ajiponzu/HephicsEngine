#version 460

layout (binding = 10) uniform UniformBufferObject {
    float deltaTime;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec3 fragColor;

void main()
{
    gl_PointSize = 10.0 * ubo.deltaTime / ubo.deltaTime;
    gl_Position = vec4(inPosition.xy, 0.0, 1.0);
    fragColor = inColor.rgb;
}