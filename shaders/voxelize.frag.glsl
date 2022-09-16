#version 450
#extension GL_ARB_separate_shader_objects : enable

#define MaterialFlag_USE_ALBEDO_TEX (1u << 0)
#define MaterialFlag_USE_NORMAL_TEX (1u << 1)
#define MaterialFlag_USE_METALLIC_TEX (1u << 2)
#define MaterialFlag_USE_ROUGHNESS_TEX (1u << 3)
#define MaterialFlag_USE_AO_TEX (1u << 4)
#define MaterialFlag_USE_EMISSIVE_TEX (1u << 5)

#define MaterialFlag_METALLIC_CHANNEL_RED (1u << 8)
#define MaterialFlag_METALLIC_CHANNEL_GREEN (1u << 9)
#define MaterialFlag_METALLIC_CHANNEL_BLUE (1u << 10)
#define MaterialFlag_METALLIC_CHANNEL_ALPHA (1u << 11)

#define MaterialFlag_ROUGHNESS_CHANNEL_RED (1u << 12)
#define MaterialFlag_ROUGHNESS_CHANNEL_GREEN (1u << 13)
#define MaterialFlag_ROUGHNESS_CHANNEL_BLUE (1u << 14)
#define MaterialFlag_ROUGHNESS_CHANNEL_ALPHA (1u << 15)

#define MaterialFlag_AO_CHANNEL_RED (1u << 16)
#define MaterialFlag_AO_CHANNEL_GREEN (1u << 17)
#define MaterialFlag_AO_CHANNEL_BLUE (1u << 18)
#define MaterialFlag_AO_CHANNEL_ALPHA (1u << 19)

#define M_PI 3.1415926535897932384626433832795

layout(set = 0, binding = 1, std140) uniform VoxelGIData{
    vec3 _voxel_gi_FrustumCenter;
    int _voxel_gi_Resolution;
    float _voxel_gi_FrustumHalfSpan;
    float _voxel_gi_Bias;
    float _voxel_gi_DiffuseMultiplier;
    float _voxel_gi_SpecularMultiplier;
} voxelGIData;

layout(set = 0, binding = 2, r32ui) uniform volatile coherent uimage3D voxelAlbedoBuffer;
layout(set = 0, binding = 3, r32ui) uniform volatile coherent uimage3D voxelNormalBuffer;
layout(set = 0, binding = 4, r32ui) uniform volatile coherent uimage3D voxelEmissiveBuffer;

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
    _voxel_gi_VoxelSize = (2.0f * voxelGIData._voxel_gi_FrustumHalfSpan) / float(voxelGIData._voxel_gi_Resolution);
    _voxel_gi_FrustumMax = voxelGIData._voxel_gi_FrustumCenter + vec3(voxelGIData._voxel_gi_FrustumHalfSpan);
    _voxel_gi_FrustumMin = voxelGIData._voxel_gi_FrustumCenter - vec3(voxelGIData._voxel_gi_FrustumHalfSpan);
    _voxel_gi_MaxMip = int(log2(voxelGIData._voxel_gi_Resolution));
}

layout(location = 1) in GS_OUT{
    vec3 ndcPosition;
    vec3 worldNormal;
    vec2 texCoord;
    vec4 triangleAABB; // xy component for min.xy, and zw for max.xy
    vec3 voxelIdx;
} gs_out;

struct Material {
    vec3 albedo;
    float metallic;
    vec3 emissive;
    float roughness;

    uint flags;
};

layout(set = 1, binding = 0) uniform PerMaterial {
    Material material;
};

layout(set = 2, binding = 0) uniform sampler2D albedoMap;
layout(set = 2, binding = 1) uniform sampler2D normalMap;
layout(set = 2, binding = 2) uniform sampler2D metallicMap;
layout(set = 2, binding = 3) uniform sampler2D roughnessMap;
layout(set = 2, binding = 4) uniform sampler2D aoMap;
layout(set = 2, binding = 5) uniform sampler2D emissiveMap;

struct PixelMaterial {
    vec3 f0;
    vec3 albedo;
    vec3 normal;
    float metallic;
    float roughness;
    float ao;
    vec3 emissive;
};


vec4 convRGBA8ToVec4(uint val) {
    return vec4(float((val & 0x000000FFu)),
    float((val & 0x0000FF00u) >> 8U),
    float((val & 0x00FF0000u) >> 16U),
    float((val & 0xFF000000u) >> 24U));
}

uint convVec4ToRGBA8(vec4 val) {
    return (uint (val.w) & 0x000000FFu) << 24U |
    (uint(val.z) & 0x000000FFu) << 16U |
    (uint(val.y) & 0x000000FFu) << 8U |
    (uint(val.x) & 0x000000FFu);
}

#define imageAtomicRGBA8Avg(imgUI, coords, valArg) do { \
    vec4 val = valArg;\
    int bound = voxelGIData._voxel_gi_Resolution - 1;\
    if (coords.x > bound || coords.y > bound || coords.z > bound) {\
        break;\
    }\
    if (coords.x < 0 || coords.y < 0 || coords.z < 0) {\
        break;\
    }\
    val.rgb *= 255.0f;\
    uint newVal = convVec4ToRGBA8(val);\
    uint prevStoredVal = 0;\
    uint curStoredVal;\
    while ((curStoredVal = imageAtomicCompSwap(imgUI, coords, prevStoredVal, newVal)) != prevStoredVal) {\
        curStoredVal = imageAtomicCompSwap(imgUI, coords, prevStoredVal, newVal);\
        prevStoredVal = curStoredVal;\
        vec4 rval = convRGBA8ToVec4(curStoredVal);\
        rval.xyz = (rval.xyz* rval.w);\
        vec4 curValF = rval + val;\
        curValF.xyz /= (curValF.w);\
        newVal = convVec4ToRGBA8(curValF);\
    }\
 } while(false)

bool bitTest(uint flags, uint mask) {
    return ((flags & mask) == mask);
}

PixelMaterial pixelMaterialCreate(Material material, vec2 texCoord) {

    PixelMaterial pixelMaterial;

    pixelMaterial.albedo = pow(material.albedo, vec3(2.2f));
    if (bitTest(material.flags, MaterialFlag_USE_ALBEDO_TEX)) {
        pixelMaterial.albedo *= pow(texture(albedoMap, texCoord).rgb, vec3(2.2f));
    }

    if (bitTest(material.flags, MaterialFlag_USE_NORMAL_TEX)) {
        pixelMaterial.normal = normalize(vec3(texture(normalMap, texCoord).rgb * 2 - 1.0f));
    }
    else {
        pixelMaterial.normal = vec3(0, 0, 1.0f);
    }

    if (bitTest(material.flags, MaterialFlag_USE_METALLIC_TEX)) {
        if (bitTest(material.flags, MaterialFlag_METALLIC_CHANNEL_RED)) pixelMaterial.metallic = texture(metallicMap, texCoord).r;
        if (bitTest(material.flags, MaterialFlag_METALLIC_CHANNEL_GREEN)) pixelMaterial.metallic = texture(metallicMap, texCoord).g;
        if (bitTest(material.flags, MaterialFlag_METALLIC_CHANNEL_BLUE)) pixelMaterial.metallic = texture(metallicMap, texCoord).b;
        if (bitTest(material.flags, MaterialFlag_METALLIC_CHANNEL_ALPHA)) pixelMaterial.metallic = texture(metallicMap, texCoord).a;
        pixelMaterial.metallic *= material.metallic;
    }
    else {
        pixelMaterial.metallic = material.metallic;
    }

    if (bitTest(material.flags, MaterialFlag_USE_ROUGHNESS_TEX)) {
        if (bitTest(material.flags, MaterialFlag_ROUGHNESS_CHANNEL_RED)) pixelMaterial.roughness = texture(roughnessMap, texCoord).r;
        if (bitTest(material.flags, MaterialFlag_ROUGHNESS_CHANNEL_GREEN)) pixelMaterial.roughness = texture(roughnessMap, texCoord).g;
        if (bitTest(material.flags, MaterialFlag_ROUGHNESS_CHANNEL_BLUE)) pixelMaterial.roughness = texture(roughnessMap, texCoord).b;
        if (bitTest(material.flags, MaterialFlag_ROUGHNESS_CHANNEL_ALPHA)) pixelMaterial.roughness = texture(roughnessMap, texCoord).a;
        pixelMaterial.roughness *= material.roughness;
    }
    else {
        pixelMaterial.roughness = material.roughness;
    }

    pixelMaterial.roughness = max(pixelMaterial.roughness, 1e-4);

    if (bitTest(material.flags, MaterialFlag_USE_AO_TEX)) {
        if (bitTest(material.flags, MaterialFlag_AO_CHANNEL_RED)) pixelMaterial.ao = texture(aoMap, texCoord).r;
        if (bitTest(material.flags, MaterialFlag_AO_CHANNEL_GREEN)) pixelMaterial.ao = texture(aoMap, texCoord).g;
        if (bitTest(material.flags, MaterialFlag_AO_CHANNEL_BLUE)) pixelMaterial.ao = texture(aoMap, texCoord).b;
        if (bitTest(material.flags, MaterialFlag_AO_CHANNEL_ALPHA)) pixelMaterial.ao = texture(aoMap, texCoord).a;
    }
    else {
        pixelMaterial.ao = 1;
    }

    if (bitTest(material.flags, MaterialFlag_USE_EMISSIVE_TEX)) {
        pixelMaterial.emissive = texture(emissiveMap, texCoord).rgb;
        pixelMaterial.emissive *= material.emissive;
    }
    else {
        pixelMaterial.emissive = material.emissive;
    }

    pixelMaterial.f0 = mix(vec3(0.04f), pixelMaterial.albedo, pixelMaterial.metallic);
    return pixelMaterial;

}

void main() {

    voxel_gi_init();

    if (any(lessThan(gs_out.ndcPosition.xy, gs_out.triangleAABB.xy)) ||
        any(greaterThan(gs_out.ndcPosition.xy, gs_out.triangleAABB.zw))) {
        discard;
    }

    ivec3 voxelIdx = ivec3(gs_out.voxelIdx);
    PixelMaterial pixelMaterial = pixelMaterialCreate(material, gs_out.texCoord);

}