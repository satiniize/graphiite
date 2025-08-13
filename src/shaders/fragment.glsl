#version 460

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_texcoord;

layout(location = 0) out vec4 FragColor;

void main()
{
    // FragColor = v_color;
    FragColor = vec4(v_texcoord.x, v_texcoord.y, 0.0, 1.0);
}
