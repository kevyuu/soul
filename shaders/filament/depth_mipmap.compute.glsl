#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_image_load_store : enable

layout(local_size_x = 1, local_size_y = 1) in;

layout(binding = 0, r32f) uniform readonly image2D depthSrcBuffer;
layout(binding = 1, r32f) uniform writeonly image2D depthDstBuffer;

void main() {
	ivec2 icoord = ivec2(gl_WorkGroupID.xy);
	vec4 val = imageLoad(depthSrcBuffer, (ivec2(2) * icoord) + ivec2(icoord.y & 1, icoord.x & 1));
	imageStore(depthDstBuffer, icoord, val);
}