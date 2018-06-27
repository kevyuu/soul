#ifdef COMPUTE_SHADER
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

struct DirectionalLight {
	mat4 shadowMatrix1;
	mat4 shadowMatrix2;
	mat4 shadowMatrix3;
	mat4 shadowMatrix4;
	vec3 direction;
	float pad1;
	vec3 color;
	float pad2;
	vec4 cascadeDepths;
};

struct AABB {
	vec3 center;
	float halfSpan;
};

layout(std140) uniform LightData{
	DirectionalLight directionalLights[4];
	int directionalLightCount;
	float pad1;
	float pad2;
	float pad3;
};

uniform vec3 voxelFrustumCenter;
uniform float voxelFrustumHalfSpan;
uniform int voxelFrustumReso;

uniform sampler3D voxelAlbedoBuffer;
uniform sampler3D voxelNormalBuffer;
layout(rgba16f) uniform writeonly image3D lightVoxelBuffer;

ivec3 worldToVoxelIdx(vec3 position, vec3 voxelFrustumMin, float voxelSize) {
	return ivec3(floor((position - voxelFrustumMin) / voxelSize));
}

vec3 voxelIdxToWorld(ivec3 voxelIdx, vec3 voxelFrustumMin, float voxelSize) {
	return voxelFrustumMin + ((vec3(voxelIdx) + vec3(0.5f)) * voxelSize);
}

float calculateDirLightVisibility(
	vec3 position,
	vec3 direction,
	float voxelSize,
	int voxelFrustumReso,
	AABB voxelFrustumAABB
) {

	vec3 frustumMax = voxelFrustumAABB.center + vec3(voxelFrustumAABB.halfSpan);
	vec3 frustumMin = voxelFrustumAABB.center - vec3(voxelFrustumAABB.halfSpan);

	float dst = 3.0f * voxelSize;
	vec3 samplePos = position + direction * dst * -1.0f;

	float occupancy = 0.0f;
	while (all(lessThanEqual(samplePos, frustumMax)) && all(greaterThanEqual(samplePos, frustumMin))) {
		ivec3 voxelIdx = worldToVoxelIdx(samplePos, frustumMin, voxelSize);
		vec3 voxelUV = vec3(voxelIdx) / vec3(voxelFrustumReso);
		float alpha = texelFetch(voxelAlbedoBuffer, voxelIdx, 0).a > 0.0f ? 1.0f : 0.0f;
		if (alpha == 1.0f) return 0.0f;

		dst += voxelSize;
		samplePos = position + direction * dst * -1.0f;
	}

	return 1.0f;

}

void main() {
	ivec3 voxelIdx = ivec3(gl_WorkGroupID.xyz);
	vec3 voxelUV = voxelIdx / vec3(gl_NumWorkGroups);
	vec4 albedo = texelFetch(voxelAlbedoBuffer, voxelIdx, 0);
	vec4 normal = normalize(texelFetch(voxelNormalBuffer, voxelIdx, 0) * 2.0f - 1.0f);

	if (albedo.a == 0.0f) {
		imageStore(lightVoxelBuffer, ivec3(gl_WorkGroupID.xyz), vec4(0.0f));
		return;
	}
	
	vec3 voxelFrustumMin = voxelFrustumCenter - vec3(voxelFrustumHalfSpan);
	float voxelSize = 2 * voxelFrustumHalfSpan / float(voxelFrustumReso);

	AABB voxelAABB;
	voxelAABB.center = voxelFrustumCenter;
	voxelAABB.halfSpan = voxelFrustumHalfSpan;

	for (int i = 0; i < directionalLightCount; i++) {
		vec3 voxelCenterWorldPos = voxelIdxToWorld(voxelIdx, voxelFrustumMin, voxelSize);

		float visibility = calculateDirLightVisibility(
			voxelCenterWorldPos, 
			directionalLights[i].direction,
			voxelSize, 
			voxelFrustumReso, 
			voxelAABB
		);

		float NdotL = dot(normal.xyz, directionalLights[i].direction * -1.0f);
		NdotL = max(NdotL, 0.0f);
		imageStore(lightVoxelBuffer, voxelIdx, vec4(visibility * albedo.rgb * directionalLights[i].color * NdotL, 1.0f));
	}

}

#endif