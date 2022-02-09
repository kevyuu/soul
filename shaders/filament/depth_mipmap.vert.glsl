#version 450

layout(set = 0, binding = 0) uniform sampler2D materialParams_depth;
layout(location = 0) in vec2 position;

void main()
{
    gl_Position = vec4(position, 0.5, 1.0);
}

