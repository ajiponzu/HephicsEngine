#version 460

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main()
{
    vec2 center = vec2(0.5f);
    float radius = 0.5f;
    vec2 coord = gl_PointCoord - center;
    float value = 1.0f - smoothstep(0.3f, 1.0f, length(coord) / radius);
    float discard_threshold = 0.5f;

    if (value < discard_threshold)
      discard;

    vec3 color = fragColor * value;
    outColor = vec4(color, value);
}