#version 450
#extension GL_ARB_separate_shader_objects : enable

#define M_PI 3.1415926535897932384626433832795

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

void main() {

    voxel_gi_init();

    int voxelFrustumReso = voxel_gi_getResolution();
    int voxelFrustumResoSqr = voxelFrustumReso * voxelFrustumReso;

    vec3 voxelIdx = vec3(
        gl_VertexIndex % voxelFrustumReso,
        (gl_VertexIndex % voxelFrustumResoSqr) / voxelFrustumReso,
        gl_VertexIndex / (voxelFrustumResoSqr)
    );

    gl_Position = vec4(voxelIdx, 1.0f);

}