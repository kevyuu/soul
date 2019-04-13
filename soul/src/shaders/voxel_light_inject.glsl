#ifdef LIB_SHADER
math.lib.glsl
light.lib.glsl
voxel_gi.lib.glsl
#endif

/**********************************************************************
// COMPUTE_SHADER
**********************************************************************/
#ifdef COMPUTE_SHADER
layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

uniform sampler3D voxelAlbedoBuffer;
uniform sampler3D voxelNormalBuffer;
uniform sampler3D voxelEmissiveBuffer;
uniform float emissiveScale;

layout(rgba16f) uniform writeonly image3D lightVoxelBuffer;


void main() {

	voxel_gi_init();
	
	ivec3 voxelIdx = ivec3(gl_GlobalInvocationID.xyz);
	vec4 albedo = texelFetch(voxelAlbedoBuffer, voxelIdx, 0);
	vec4 normal = normalize(texelFetch(voxelNormalBuffer, voxelIdx, 0) * 2.0f - 1.0f);
	vec4 emissive = texelFetch(voxelEmissiveBuffer, voxelIdx, 0);

	if (albedo.a == 0.0f) {
		imageStore(lightVoxelBuffer, ivec3(gl_WorkGroupID.xyz), vec4(0.0f));
		return;
	}

	vec3 luminance = vec3(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < directionalLightCount; i++) {
		vec3 L = directionalLights[i].direction * -1.0f;

		float visibility = voxel_gi_occlusionRayTrace(
			voxelAlbedoBuffer,
			voxel_gi_getWorldPosition(voxelIdx),
			L
		);

		float NdotL = dot(normal.xyz, L);
		NdotL = max(NdotL, 0.0f);
		luminance += visibility * albedo.rgb * directionalLights[i].color * directionalLights[i].preExposedIlluminance * NdotL;
	}
	luminance += emissive.rgb * emissiveScale;
	imageStore(lightVoxelBuffer, voxelIdx, vec4(luminance, 1.0f));

}

#endif //COMPUTE_SHADER