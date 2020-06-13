#version 450
#extension GL_ARB_separate_shader_objects : enable

#define M_PI 3.1415926535897932384626433832795

layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

layout(set = 0, binding = 0, std140) uniform VoxelGIData{
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

layout(set = 0, binding = 1, std140) uniform CameraData{
    mat4 _camera_projection;
    mat4 _camera_view;
    mat4 _camera_projectionView;
    mat4 _camera_invProjectionView;

    mat4 _camera_prevProjection;
    mat4 _camera_prevView;
    mat4 _camera_prevProjectionView;

    vec3 _camera_cameraPosition;
    float _camera_exposure;

};

layout(set = 0, binding=2, rgba8) uniform readonly image3D voxelAlbedoBuffer;
layout(set = 0, binding=3, rgba8) uniform readonly image3D voxelEmissiveBuffer;
layout(set = 0, binding=4, rgba8) uniform readonly image3D voxelNormalBuffer;

layout(location = 0) out GS_OUT{
    vec4 color;
} gs_out;

void main() {

    voxel_gi_init();
    vec4 albedo = imageLoad(voxelAlbedoBuffer, ivec3(gl_in[0].gl_Position.xyz));
    vec4 normal = imageLoad(voxelNormalBuffer, ivec3(gl_in[0].gl_Position.xyz));
    vec4 emissive = imageLoad(voxelNormalBuffer, ivec3(gl_in[0].gl_Position.xyz));

    if (color.a == 0.0f) return;

    vec3 voxelFrustumMin = voxel_gi_getFrustumMin();
    float voxelSize = voxel_gi_getVoxelSize();

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
    if (albedo.a > 0) {
        albedo.a = 1.0f;
    }
    gs_out.color = albedo;

    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 4; j++) {
            vec4 vertexOut = vec4(voxelCenter, 1.0f) + cubeVertices[cubeIndices[i * 4 + j]] * voxelSize;
            gl_Position = _camera_projectionView * vertexOut;
            EmitVertex();
        }
        EndPrimitive();
    }
}