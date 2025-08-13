#version 460

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_texcoord;

layout(binding = 0) uniform sampler2D myTextureSampler;

layout(location = 0) out vec4 FragColor;

void main()
{
    // FragColor = v_color;
    // FragColor = vec4(v_texcoord.x, v_texcoord.y, 0.0, 1.0);
    FragColor = texture(myTextureSampler, v_texcoord);
}
