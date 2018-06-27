#define M_PI 3.1415926535897932384626433832795

/**********************************************************************
// VERTEX_SHADER
**********************************************************************/
#ifdef VERTEX_SHADER

uniform int voxelFrustumReso;

void main() {

	int voxelFrustumResoSqr = voxelFrustumReso * voxelFrustumReso;

	vec3 voxelIdx = vec3(
		gl_VertexID % voxelFrustumReso,
		(gl_VertexID % voxelFrustumResoSqr) / voxelFrustumReso,
		gl_VertexID / (voxelFrustumResoSqr)
	);

	gl_Position = vec4(voxelIdx, 1.0f);

}
#endif //VERTEX_SHADER

/**********************************************************************
// GEOMETRY_SHADER
**********************************************************************/
#ifdef GEOMETRY_SHADER
layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

layout(binding=0, rgba16f) uniform readonly image3D voxelBuffer;
uniform int voxelFrustumReso;
uniform vec3 voxelFrustumCenter;
uniform float voxelFrustumHalfSpan;

layout(std140) uniform SceneData{
	mat4 projection;
	mat4 view;
};

out GS_OUT{
	vec4 color;
} gs_out;

void main() {

	vec4 color = imageLoad(voxelBuffer, ivec3(gl_in[0].gl_Position.xyz));
	if (color.a == 0.0f) return;
	
	vec3 voxelFrustumMin = voxelFrustumCenter - vec3(voxelFrustumHalfSpan);
	float voxelSize = float(2 * voxelFrustumHalfSpan) / float(voxelFrustumReso);
	
	const vec4 cubeVertices[8] = vec4[8]
	(
		vec4(0.5f, 0.5f, 0.5f, 0.0f),
		vec4(0.5f, 0.5f, -0.5f, 0.0f),
		vec4(0.5f, -0.5f, 0.5f, 0.0f),
		vec4(0.5f, -0.5f, -0.5f, 0.0f),
		vec4(-0.5f, 0.5f, 0.5f, 0.0f),
		vec4(-0.5f, 0.5f, -0.5f, 0.0f),
		vec4(-0.5f, -0.5f, 0.5f, 0.0f),
		vec4(-0.5f, -0.5f, -0.5f, 0.0f)
	);

	const int cubeIndices[24] = int[24]
	(
		0, 2, 1, 3, // right
		6, 4, 7, 5, // left
		5, 4, 1, 0, // up
		6, 7, 2, 3, // down
		4, 6, 0, 2, // front
		1, 3, 5, 7  // back
	);

	vec3 voxelCenter = (gl_in[0].gl_Position.xyz + vec3(0.5f)) * voxelSize + voxelFrustumMin;
	gs_out.color = color;

	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 4; j++) {
			vec4 vertexOut = vec4(voxelCenter, 1.0f) + cubeVertices[cubeIndices[i * 4 + j]] * voxelSize;
			gl_Position = projection * view * vertexOut;
			EmitVertex();
		}
		EndPrimitive();
	}

}

#endif // GEOMETRY_SHADER

/**********************************************************************
// FRAGMENT_SHADER
**********************************************************************/
#ifdef FRAGMENT_SHADER

in GS_OUT{
	vec4 color;
} gs_out;

out vec4 fragColor;

void main() {
	fragColor = gs_out.color;
}
#endif // FRAGMENT_SHADER