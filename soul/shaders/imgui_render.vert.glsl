#version 450
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in uint aColor;

layout(set = 0, binding = 0) uniform Transform {
    vec2 uScale;
    vec2 uTranslate;
} transform;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out struct {
    vec4 Color;
    vec2 UV;
} Out;

void main()
{
    Out.Color = vec4((aColor & 0xFF) / 255.0f, (aColor >> 8 & 0xFF) / 255.0f, (aColor >> 16 & 0xFF) / 255.0f, (aColor >> 32 & 0xFF) / 255.0f);
    Out.UV = aUV;
    gl_Position = vec4(aPos * transform.uScale + transform.uTranslate, 0, 1);
}
