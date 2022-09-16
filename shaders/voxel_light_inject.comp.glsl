#version 450
#extension GL_ARB_separate_shader_objects : enable

#define M_PI 3.1415926535897932384626433832795

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

layout(set = 0, binding = 0) uniform usampler3D voxelAlbedoBuffer;
layout(set = 0, binding = 1) uniform usampler3D voxelNormalBuffer;
layout(set = 0, binding = 2) uniform usampler3D voxelEmissiveBuffer;

layout(set = 0, binding = 3, rgba16f) uniform writeonly image3D lightVoxelBuffer;

layout(set = 0, binding = 4, std140) uniform VoxelGIData{
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

const int _VOXEL_GI_DIFFUSE_SAMPLE = 6;

const vec3 _VOXEL_GI_DIFFUSE_CONE_DIRECTIONS[_VOXEL_GI_DIFFUSE_SAMPLE] =
{
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.5f, 0.866025f),
    vec3(0.823639f, 0.5f, 0.267617f),
    vec3(0.509037f, 0.5f, -0.7006629f),
    vec3(-0.50937f, 0.5f, -0.7006629f),
    vec3(-0.823639f, 0.5f, 0.267617f)
};

const float _VOXEL_GI_DIFFUSE_CONE_WEIGHTS[_VOXEL_GI_DIFFUSE_SAMPLE] =
{
    M_PI / 4.0f,
    3.0f * M_PI / 20.0f,
    3.0f * M_PI / 20.0f,
    3.0f * M_PI / 20.0f,
    3.0f * M_PI / 20.0f,
    3.0f * M_PI / 20.0f,
};

const float _VOXEL_GI_DIFFUSE_CONE_TANGENT = 0.57735f;

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

vec4 voxel_gi_coneTrace(
    sampler3D voxelTex,
    vec3 position,
    vec3 direction,
    float coneTangent) {

    vec3 startPos = position + direction * _voxel_gi_Bias * _voxel_gi_VoxelSize;

    vec3 color = vec3(0.0f);
    float alpha = 0.0f;
    vec3 samplePos = startPos;

    while (voxel_gi_insideFrustum(samplePos) && alpha < 0.99f) {
        float radius = max(_voxel_gi_VoxelSize / 2, coneTangent * length(samplePos - position));
        float mip = min(log2(2 * radius / _voxel_gi_VoxelSize), _voxel_gi_MaxMip);

        vec3 samplePosUVW = voxel_gi_getTexCoord(samplePos);
        vec4 sampleColor = textureLod(voxelTex, samplePosUVW, mip);
        color += (1 - alpha) * sampleColor.rgb;
        alpha += (1 - alpha) * sampleColor.a;

        samplePos += radius * direction;
    }

    return vec4(color, alpha);
}

float voxel_gi_occlusionRayTrace(
    usampler3D voxelTex,
    vec3 position,
    vec3 direction) {

    vec3 startPos = position + direction * 2 * _voxel_gi_VoxelSize;

    vec3 samplePos = startPos;

    float occupancy = 0.0f;
    while (voxel_gi_insideFrustum(samplePos)) {
        ivec3 voxelIdx = voxel_gi_getVoxelIndex(samplePos);
        float alpha = texelFetch(voxelTex, voxelIdx, 0).a > 0 ? 1.0f : 0.0f;

        if (alpha == 1.0f) return 0.0f;

        samplePos += _voxel_gi_VoxelSize * direction;

    }

    return 1.0f;

}

vec4 voxel_gi_diffuseConeTrace(
    sampler3D voxelTex,
    vec3 position,
    vec3 normal) {

    vec3 ref = vec3(0.0f, 1.0f, 0.0f);

    if (abs(dot(normal, ref)) == 1.0f) {
        ref = vec3(0.0f, 0.0f, 1.0f);
    }

    vec3 tangent = cross(normal, ref);
    vec3 bitangent = cross(normal, tangent);

    mat3 rotationMat = mat3(tangent, normal, bitangent);

    vec4 color = vec4(0.0f);
    for (int i = 0; i < _VOXEL_GI_DIFFUSE_SAMPLE; i++) {
        vec3 L = normalize(rotationMat * _VOXEL_GI_DIFFUSE_CONE_DIRECTIONS[i]);
        vec4 diffuseColor = voxel_gi_coneTrace(voxelTex, position, L, _VOXEL_GI_DIFFUSE_CONE_TANGENT);
        color += diffuseColor;
    }

    color.xyz *= 2 * M_PI;
    color /= 6.0f;

    return color;

}

struct DirectionalLight {
    mat4 shadowMatrix[4];
    vec3 direction;
    float bias;
    vec3 color;
    float preExposedIlluminance;
    vec4 cascadeDepths;
};

layout(set = 0, binding = 5, std140) uniform Light {
    DirectionalLight dirLight;
};

void main() {

    voxel_gi_init();

    ivec3 voxelIdx = ivec3(gl_GlobalInvocationID.xyz);
    vec4 albedo = texelFetch(voxelAlbedoBuffer, voxelIdx, 0) / vec4(255.0f);
    vec4 normal = normalize(texelFetch(voxelNormalBuffer, voxelIdx, 0) * 2.0f - 1.0f);
    vec4 emissive = texelFetch(voxelEmissiveBuffer, voxelIdx, 0);

    if (albedo.a == 0.0f) {
        imageStore(lightVoxelBuffer, voxelIdx, vec4(0.0f));
        return;
    }

    vec3 luminance = vec3(0.0f, 0.0f, 0.0f);
    vec3 L = dirLight.direction * -1.0f;

    float visibility = voxel_gi_occlusionRayTrace(
        voxelAlbedoBuffer,
        voxel_gi_getWorldPosition(voxelIdx),
        L
    );

    float NdotL = dot(normal.xyz, L);
    NdotL = max(NdotL, 0.0f);
    luminance += visibility * albedo.rgb * dirLight.color * dirLight.preExposedIlluminance * NdotL;
    //  += emissive.rgb;
    imageStore(lightVoxelBuffer, voxelIdx, vec4(luminance, 1.0f));

}