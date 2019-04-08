#ifdef LIB_SHADER
math.lib.glsl
voxel_gi.lib.glsl
brdf.lib.glsl
#endif

/**********************************************************************
// VERTEX_SHADER
**********************************************************************/
#ifdef VERTEX_SHADER

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aBinormal;
layout(location = 4) in vec3 aTangent;

uniform mat4 model;

out VS_OUT{
	vec3 worldPosition;
	vec3 worldNormal;
	vec2 texCoord;
} vs_out;

void main() {
	gl_Position = model * vec4(aPos, 1.0f);
	vs_out.worldPosition = vec3((model * vec4(aPos, 1.0f)).xyz);
	mat3 rotMat = mat3(transpose(inverse(model)));
	vs_out.worldNormal = rotMat * aNormal;
	vs_out.texCoord = aTexCoord;
}
#endif //VERTEX SHADER

/**********************************************************************
// GEOMETRY_SHADER
**********************************************************************/
#ifdef GEOMETRY_SHADER
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in VS_OUT{
	vec3 worldPosition;
	vec3 worldNormal;
	vec2 texCoord;
} vs_out[];

out GS_OUT{
	vec3 ndcPosition;
	vec3 worldNormal;
	vec2 texCoord;
	vec4 triangleAABB; // xy component for min.xy, and zw for max.xy
	vec3 voxelIdx;
} gs_out;

uniform mat4 projectionView[3];
uniform mat4 inverseProjectionView[3];

void main() {
	
	voxel_gi_init();

	vec3 side10 = (gl_in[0].gl_Position - gl_in[1].gl_Position).xyz;
	vec3 side21 = (gl_in[1].gl_Position - gl_in[2].gl_Position).xyz;
	vec3 side02 = (gl_in[2].gl_Position - gl_in[0].gl_Position).xyz;

	vec3 faceNormal = normalize(cross(side10, side21));

	// swizzle xyz component according the dominant axis.
	// the dominant axis should be on z axis (the projection axis)
	float absNormalX = abs(faceNormal.x);
	float absNormalY = abs(faceNormal.y);
	float absNormalZ = abs(faceNormal.z);

	int dominantAxis = 2;
	if (absNormalX >= absNormalY && absNormalX >= absNormalZ) {
		dominantAxis = 0;
	}
	else if (absNormalY >= absNormalX && absNormalY >= absNormalZ) {
		dominantAxis = 1;
	}

	vec4 outVertex[3] = vec4[3](
		projectionView[dominantAxis] * vec4(vs_out[0].worldPosition, 1.0f),
		projectionView[dominantAxis] * vec4(vs_out[1].worldPosition, 1.0f),
		projectionView[dominantAxis] * vec4(vs_out[2].worldPosition, 1.0f));

	vec2 texCoord[3];
	for (int i = 0; i < 3; i++) {
		texCoord[i] = vs_out[i].texCoord;
	}

	vec2 halfVoxelPxSize = vec2(1.0f / float(voxel_gi_getResolution()));

	vec4 triangleAABB;
	triangleAABB.xy = outVertex[0].xy;
	triangleAABB.zw = outVertex[0].xy;
	for (int i = 1; i < 3; i++) {
		triangleAABB.xy = min(triangleAABB.xy, outVertex[i].xy);
		triangleAABB.zw = max(triangleAABB.zw, outVertex[i].xy);
	}
	triangleAABB.xy -= halfVoxelPxSize;
	triangleAABB.zw += halfVoxelPxSize;

	// Calculate the plane of the triangle in the form of ax + by + cz + w = 0. xyz is the normal, w is the offset. 
	vec4 trianglePlane;
	trianglePlane.xyz = cross(outVertex[1].xyz - outVertex[0].xyz, outVertex[2].xyz - outVertex[0].xyz);
	trianglePlane.w = -dot(trianglePlane.xyz, outVertex[0].xyz);

	// change winding, otherwise there are artifacts for the back faces.
	if (dot(trianglePlane.xyz, vec3(0.0, 0.0, 1.0)) < 0.0)
	{
		vec4 vertexTemp = outVertex[2];
		vec2 texCoordTemp = texCoord[2];

		outVertex[2] = outVertex[1];
		texCoord[2] = texCoord[1];

		outVertex[1] = vertexTemp;
		texCoord[1] = texCoordTemp;
	}

	if (trianglePlane.z == 0.0f) return;
	
	// The following code is an implementation of a conservative rasterization algorithm 
	// describe in https://developer.nvidia.com/gpugems/GPUGems2/gpugems2_chapter42.html
	// Refer to the second algorithm in the article.
	// TODO: Compare the performance with the first algorithm
	vec3 plane[3];
	plane[0] = cross(outVertex[0].xyw - outVertex[2].xyw, outVertex[2].xyw);
	plane[1] = cross(outVertex[1].xyw - outVertex[0].xyw, outVertex[0].xyw);
	plane[2] = cross(outVertex[2].xyw - outVertex[1].xyw, outVertex[1].xyw);

	plane[0].z -= dot(halfVoxelPxSize, abs(plane[0].xy));
	plane[1].z -= dot(halfVoxelPxSize, abs(plane[1].xy));
	plane[2].z -= dot(halfVoxelPxSize, abs(plane[2].xy));

	vec3 intersection[3];
	intersection[0] = cross(plane[0], plane[1]);
	intersection[1] = cross(plane[1], plane[2]);
	intersection[2] = cross(plane[2], plane[0]);

	outVertex[0].xy = intersection[0].xy / intersection[0].z;
	outVertex[1].xy = intersection[1].xy / intersection[1].z;
	outVertex[2].xy = intersection[2].xy / intersection[2].z;
	
	// Get the z value using the plane equation:
	// z = -(ax + by + w) / c;
	outVertex[0].z = -(trianglePlane.x * outVertex[0].x + trianglePlane.y * outVertex[0].y + trianglePlane.w) / trianglePlane.z;
	outVertex[1].z = -(trianglePlane.x * outVertex[1].x + trianglePlane.y * outVertex[1].y + trianglePlane.w) / trianglePlane.z;
	outVertex[2].z = -(trianglePlane.x * outVertex[2].x + trianglePlane.y * outVertex[2].y + trianglePlane.w) / trianglePlane.z;

	outVertex[0].w = 1.0f;
	outVertex[1].w = 1.0f;
	outVertex[2].w = 1.0f;

	for (int i = 0; i < 3; i++) {
		gl_Position = outVertex[i];

		vec4 worldPosition = inverseProjectionView[dominantAxis] * outVertex[i];
		worldPosition /= worldPosition.w;
		gs_out.ndcPosition = outVertex[i].xyz;
		gs_out.voxelIdx = voxel_gi_getTexCoord(worldPosition.xyz) * voxel_gi_getResolution();
		gs_out.worldNormal = vs_out[i].worldNormal;
		gs_out.texCoord = texCoord[i];
		gs_out.triangleAABB = triangleAABB;
		EmitVertex();
	}
	EndPrimitive();

}

#endif // GEOMETRY_SHADER

/**********************************************************************
// FRAGMENT_SHADER
**********************************************************************/
#ifdef FRAGMENT_SHADER

in GS_OUT{
	vec3 ndcPosition;
	vec3 worldNormal;
	vec2 texCoord;
	vec4 triangleAABB; // xy component for min.xy, and zw for max.xy
	vec3 voxelIdx;
} gs_out;


layout(r32ui) uniform volatile coherent uimage3D voxelAlbedoBuffer;
layout(r32ui) uniform volatile coherent uimage3D voxelNormalBuffer;
layout(r32ui) uniform volatile coherent uimage3D voxelEmissiveBuffer;

uniform Material material;

vec4 convRGBA8ToVec4(uint val) {
	return vec4(float((val & 0x000000FF)), 
		float((val & 0x0000FF00) >> 8U), 
		float((val & 0x00FF0000) >> 16U), 
		float((val & 0xFF000000) >> 24U));
}

uint convVec4ToRGBA8(vec4 val) {
	return (uint (val.w) & 0x000000FF) << 24U | 
		(uint(val.z) & 0x000000FF) << 16U | 
		(uint(val.y) & 0x000000FF) << 8U | 
		(uint(val.x) & 0x000000FF);
}
void imageAtomicRGBA8Avg(layout(r32ui) volatile coherent uimage3D imgUI, ivec3 coords, vec4 val) {
	
	int bound = voxel_gi_getResolution() - 1;
	if (coords.x > bound || coords.y > bound || coords.z > bound) {
		return;
	}
	if (coords.x < 0 || coords.y < 0 || coords.z < 0) {
		return;
	}
	
	val.rgb *= 255.0f; // Optimise following calculations
	uint newVal = convVec4ToRGBA8(val);
	uint prevStoredVal = 0; 
	uint curStoredVal;
	// Loop as long as destination value gets changed by other threads
	while ((curStoredVal = imageAtomicCompSwap(imgUI, coords, prevStoredVal, newVal)) != prevStoredVal) {
		prevStoredVal = curStoredVal;
		vec4 rval = convRGBA8ToVec4(curStoredVal);
		rval.xyz = (rval.xyz* rval.w); // Denormalize
		vec4 curValF = rval + val; // Add new value
		curValF.xyz /= (curValF.w); // Renormalize
		newVal = convVec4ToRGBA8(curValF);
	}
}

void main() {
	
	voxel_gi_init();

	if (any(lessThan(gs_out.ndcPosition.xy, gs_out.triangleAABB.xy)) || 
		any(greaterThan(gs_out.ndcPosition.xy, gs_out.triangleAABB.zw))) {
		discard;
	}

	ivec3 voxelIdx = ivec3(gs_out.voxelIdx);
	PixelMaterial pixelMaterial = pixelMaterialCreate(material, gs_out.texCoord);

	imageAtomicRGBA8Avg(voxelAlbedoBuffer, voxelIdx, vec4(pixelMaterial.albedo, 1.0f));
	imageAtomicRGBA8Avg(voxelNormalBuffer, voxelIdx, vec4(gs_out.worldNormal * 0.5f + 0.5f, 1.0f));
	imageAtomicRGBA8Avg(voxelEmissiveBuffer, voxelIdx, vec4(pixelMaterial.emissive, 1.0f));
}
#endif // FRAGMENT_SHADER