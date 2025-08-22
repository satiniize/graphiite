#version 460

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_texcoord;

layout(location = 0) out vec4 v_color;
layout(location = 1) out vec2 v_texcoord;

layout(std140, set = 1, binding = 0) uniform UniformBlock {
    mat4 mvp_matrix;
    float time;
    float offset;
};

void main()
{
    gl_Position = mvp_matrix * vec4(a_position * (1.0f + 0.2f * sin(8.0f * time + -offset * 0.2f)) + vec3(0.0f, 0.1f * sin(8.0f * time + -offset / 5.0f), 0.0f), 1.0f);
    v_color = a_color;
    v_texcoord = a_texcoord;
}
