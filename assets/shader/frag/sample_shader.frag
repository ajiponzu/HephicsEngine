#version 460

layout(binding = 3) uniform sampler2D texSampler;
layout(binding = 4) uniform Timer {
  float time;
} timer;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragPosition;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = abs(cos(timer.time * 2.f)) * texture(texSampler, fragTexCoord);
}