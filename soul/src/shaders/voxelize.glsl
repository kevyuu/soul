#ifdef LIB_SHADER
math.lib.glsl
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

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out VS_OUT{
	vec3 worldPosition;
	vec3 worldNormal;
	vec3 worldTangent;
	vec3 worldBinormal;
	vec2 texCoord;
} vs_out;

void main() {
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
	vs_out.worldPosition = vec3((model * vec4(aPos, 1.0f)).xyz);
	mat3 rotMat = mat3(transpose(inverse(model)));
	vs_out.worldNormal = rotMat * aNormal;
	vs_out.worldTangent = rotMat * aTangent;
	vs_out.worldBinormal = rotMat * aBinormal;
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
	vec3 worldTangent;
	vec3 worldBinormal;
	vec2 texCoord;
} vs_out[];

out GS_OUT{
	vec3 worldPosition;
	vec3 worldNormal;
	vec3 worldTangent;
	vec3 worldBinormal;
	vec2 texCoord;
} gs_out;

uniform int voxelFrustumReso;

void main() {
	
	vec3 side10 = (gl_in[0].gl_Position - gl_in[1].gl_Position).xyz;
	vec3 side21 = (gl_in[1].gl_Position - gl_in[2].gl_Position).xyz;
	vec3 side02 = (gl_in[2].gl_Position - gl_in[0].gl_Position).xyz;

	vec3 faceNormal = normalize(cross(side10, side21));

	vec3 outVertex[3] = vec3[3](
		gl_in[0].gl_Position, 
		gl_in[1].gl_Position, 
		gl_in[2].gl_Position);
	
	float absNormalX = abs(faceNormal.x);
	float absNormalY = abs(faceNormal.y);
	float absNormalZ = abs(faceNormal.z);

	if (absNormalX >= absNormalY && absNormalX >= absNormalZ) {
		for (int i = 0; i < 3; i++) {
			outVertex[i] = outVertex[i].zyx;
		}
	}
	else if (absNormalY >= absNormalX && absNormalY >= absNormalZ) {
		for (int i = 0; i < 3; i++) {
			outVertex[i] = outVertex[i].xzy;
		}
	}

	side10 = outVertex[0] - outVertex[1];
	side21 = outVertex[1] - outVertex[2];
	side02 = outVertex[2] - outVertex[0];

	float voxelPxSize = 1 / float(voxelFrustumReso);
	outVertex[0] += normalize(side10 - side02) * voxelPxSize;
	outVertex[1] += normalize(side21 - side10) * voxelPxSize;
	outVertex[2] += normalize(side02 - side21) * voxelPxSize;

	for (int i = 0; i < 3; i++) {
		gl_Position = vec4(outVertex[i], 1.0f);
		gs_out.worldPosition = vs_out[i].worldPosition;
		gs_out.worldNormal = vs_out[i].worldNormal;
		gs_out.worldTangent = vs_out[i].worldTangent;
		gs_out.worldBinormal = vs_out[i].worldBinormal;
		gs_out.texCoord = vs_out[i].texCoord;
		EmitVertex();
	}
	EndPrimitive();
}

#endif // GEOMETRY_SHADER

/**********************************************************************
// FRAGMENT_SHADER
**********************************************************************/
#ifdef FRAGMENT_SHADER

struct Material {
	sampler2D albedoMap;
	sampler2D normalMap;
	sampler2D metallicMap;
	sampler2D roughnessMap;
	sampler2D aoMap;
};

in GS_OUT{
	vec3 worldPosition;
	vec3 worldNormal;
	vec3 worldTangent;
	vec3 worldBinormal;
	vec2 texCoord;
} gs_out;

layout(r32ui) uniform volatile coherent uimage3D voxelAlbedoBuffer;
layout(r32ui) uniform volatile coherent uimage3D voxelNormalBuffer;
uniform vec3 voxelFrustumCenter;
uniform int voxelFrustumReso;
uniform float voxelFrustumHalfSpan;
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
	
	if (coords.x > 255 || coords.y > 255 || coords.z > 255) {
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

	vec3 voxelMin = voxelFrustumCenter - vec3(voxelFrustumHalfSpan);
	ivec3 voxelIdx = ivec3((gs_out.worldPosition - voxelMin) / (2 * voxelFrustumHalfSpan / voxelFrustumReso));
	vec3 albedo = pow(texture(material.albedoMap, gs_out.texCoord).rgb, vec3(2.2));
	vec3 normal = gs_out.worldNormal;

	imageAtomicRGBA8Avg(voxelAlbedoBuffer, voxelIdx, vec4(albedo, 1.0f));
	imageAtomicRGBA8Avg(voxelNormalBuffer, voxelIdx, vec4(normal * 0.5f + 0.5f, 1.0f));

}
#endif // FRAGMENT_SHADER