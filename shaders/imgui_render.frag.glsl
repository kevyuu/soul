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

layout(location = 0) out vec4 fColor;

layout(location = 0) in struct {
    vec4 Color;
    vec2 UV;
} In;   

void main()
{
    fColor = In.Color * texture(sampler2D(global_textures[PushConstants.texture_descriptor_id], global_samplers[PushConstants.sampler_descriptor_id]), In.UV.st);
}
