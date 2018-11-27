
/**********************************************************************
// COMPUTE_SHADER
**********************************************************************/
#ifdef COMPUTE_SHADER

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding=0, rgba16f) uniform readonly image3D voxelSrcBuffer;
layout(binding=1, rgba16f) uniform writeonly image3D voxelDstBuffer;

void main() {

	const ivec3 offset[8] = ivec3[8]
	(
		ivec3(0, 0, 0),
		ivec3(0, 0, 1),
		ivec3(0, 1, 0),
		ivec3(0, 1, 1),
		ivec3(1, 0, 0),
		ivec3(1, 0, 1),
		ivec3(1, 1, 0),
		ivec3(1, 1, 1)
	);

	vec4 average = vec4(0.0f);
	ivec3 voxelTargetIdx = ivec3(gl_WorkGroupID.xyz);
	ivec3 voxelSrcBaseIdx = voxelTargetIdx * 2;
	float divisor = 0.0f;
	for (int i = 0; i < 8; i++) {
		vec4 val = imageLoad(voxelSrcBuffer, voxelSrcBaseIdx + offset[i]);
		average += val;
	}
	average *= 0.125f;
	imageStore(voxelDstBuffer, voxelTargetIdx, average);
}

#endif // COMPUTE_SHADER