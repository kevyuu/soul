#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in VS_OUT{
    vec3 worldPosition;
    vec3 worldNormal;
    vec2 texCoord;
} vs_out[];

layout(location = 1) out GS_OUT{
    vec3 ndcPosition;
    vec3 worldNormal;
    vec2 texCoord;
    vec4 triangleAABB; // xy component for min.xy, and zw for max.xy
    vec3 voxelIdx;
} gs_out;

layout(set = 0, binding = 0) uniform VoxelizeMatrixes {
    mat4 projectionView[3];
    mat4 inverseProjectionView[3];
};

layout(set = 0, binding = 1, std140) uniform VoxelGIData{
    vec3 _voxel_gi_FrustumCenter;
    int _voxel_gi_Resolution;
    float _voxel_gi_FrustumHalfSpan;
    float _voxel_gi_Bias;
    float _voxel_gi_DiffuseMultiplier;
    float _voxel_gi_SpecularMultiplier;
};

float _voxel_gi_VoxelSize;
vec3 _voxel_gi_FrustumMax;
vec3 _voxel_gi_FrustumMin;
int _voxel_gi_MaxMip;

void voxel_gi_init() {
    _voxel_gi_VoxelSize = (2.0f * _voxel_gi_FrustumHalfSpan) / float(_voxel_gi_Resolution);
    _voxel_gi_FrustumMax = _voxel_gi_FrustumCenter + vec3(_voxel_gi_FrustumHalfSpan);
    _voxel_gi_FrustumMin = _voxel_gi_FrustumCenter - vec3(_voxel_gi_FrustumHalfSpan);
    _voxel_gi_MaxMip = int(log2(_voxel_gi_Resolution));
}

vec3 voxel_gi_getFrustumCenter() {
    return _voxel_gi_FrustumCenter;
}

int voxel_gi_getResolution() {
    return _voxel_gi_Resolution;
}

float voxel_gi_getVoxelSize() {
    return _voxel_gi_VoxelSize;
}

float voxel_gi_getFrustumHalfSpan() {
    return _voxel_gi_FrustumHalfSpan;
}

vec3 voxel_gi_getFrustumMin() {
    return _voxel_gi_FrustumMin;
}

vec3 voxel_gi_getFrustumMax() {
    return _voxel_gi_FrustumMax;
}

float voxel_gi_getBias() {
    return _voxel_gi_Bias;
}

float voxel_gi_getDiffuseMultiplier() {
    return _voxel_gi_DiffuseMultiplier;
}

float voxel_gi_getSpecularMultiplier() {
    return _voxel_gi_SpecularMultiplier;
}

vec3 voxel_gi_getTexCoord(vec3 worldPos) {
    return (worldPos - _voxel_gi_FrustumMin) / (2 * _voxel_gi_FrustumHalfSpan);
}

ivec3 voxel_gi_getVoxelIndex(vec3 position) {
    return ivec3(floor((position - _voxel_gi_FrustumMin) / _voxel_gi_VoxelSize));
}

vec3 voxel_gi_getWorldPosition(ivec3 voxelIndex) {
    return _voxel_gi_FrustumMin + ((vec3(voxelIndex) + vec3(0.5f)) * _voxel_gi_VoxelSize);
}

bool voxel_gi_insideFrustum(vec3 position) {
    return (all(lessThanEqual(position, _voxel_gi_FrustumMax)) && all(greaterThanEqual(position, _voxel_gi_FrustumMin)));
}

vec4 adjust_depth(vec4 initial_position) {
    vec4 position = initial_position;
    position.z = (position.z + position.w) / 2.0;
    return position;
}

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
        gl_Position = adjust_depth(outVertex[i]);

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