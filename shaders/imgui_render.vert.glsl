#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_separate_shader_objects : enable

struct Transform {
    vec2 uScale;
    vec2 uTranslate;
};

layout(set = 0, binding = 0) buffer TransformSSBO { Transform transform; } global_transforms[];

layout(set = 1, binding = 0) uniform sampler global_samplers[];

layout(set = 2, binding = 0) uniform texture2D global_textures[];

layout(set = 3, binding = 0, rgba8) uniform image2D global_images_2d_rgba8[];
layout(set = 3, binding = 0, rgba16f) uniform image2D global_images_2d_rgba16f[];
layout(set = 3, binding = 0, rgba32f) uniform image2D global_images_2d_rgba32f[];
layout(set = 3, binding = 0, r32f) uniform image2D global_images_2d_r32f[];

layout(push_constant) uniform constants
{
    uint transform_descriptor_id;
    uint sampler_descriptor_id;
    uint texture_descriptor_id;
} PushConstants;

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in uint aColor;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out struct {
    vec4 Color;
    vec2 UV;
} Out;

void main()
{
    Transform transform = global_transforms[PushConstants.transform_descriptor_id].transform;
    Out.Color = vec4((aColor & 0xFF) / 255.0f, (aColor >> 8 & 0xFF) / 255.0f, (aColor >> 16 & 0xFF) / 255.0f, (aColor >> 24 & 0xFF) / 255.0f);
    Out.UV = aUV;
    gl_Position = vec4(aPos * transform.uScale + transform.uTranslate, 0, 1);
}
