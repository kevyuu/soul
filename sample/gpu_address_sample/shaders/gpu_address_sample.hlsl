#include "gpu_address_sample_type.hlsl"

bool is_ray_intersect_aabb(soulsl::float3 ray_origin, soulsl::float3 ray_direction, 
    soulsl::float3 aabb_min, soulsl::float3 aabb_max)
{
    soulsl::float3 extent = aabb_max - aabb_min;
    soulsl::float3 inv_dir = 1.0 / ray_direction;

    soulsl::float3 t1 = (aabb_min - ray_origin) * inv_dir;
    soulsl::float3 t2 = (aabb_max - ray_origin) * inv_dir;

    float tmin = max(min(t1.x, t2.x), max(min(t1.y, t2.y), min(t1.z, t2.z)));
    float tmax = min(max(t1.x, t2.x), min(max(t1.y, t2.y), max(t1.z, t2.z)));
    return tmax >= tmin;
}

[[vk::push_constant]]
PushConstant push_constant;

[numthreads(WORK_GROUP_SIZE_X, WORK_GROUP_SIZE_Y, 1)]
void cs_main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    int2 texel_coord = dispatch_thread_id.xy;

    GPUScene scene = vk::RawBufferLoad<GPUScene>(push_constant.scene_gpu_address.id, 4);
    float3 cube_color = vk::RawBufferLoad<float3>(push_constant.scene_gpu_address.id + 12, 4);

    const float2 pixel_center = float2(texel_coord.xy) + float2(0.5, 0.5);
    const float2 in_uv = pixel_center / float2(push_constant.dimension);
    float2 d = in_uv * 2.0 - 1.0;

    float4 target = mul(scene.projection_inverse, float4(d.x, d.y, 1, 1));
    float3 origin = mul(scene.view_inverse, float4(0, 0, 0, 1)).xyz;
    float3 direction = mul(scene2.view_inverse, float4(normalize(target.xyz), 0)).xyz;

    float3 output_color = scene.sky_color;
    if (is_ray_intersect_aabb(origin, direction, float3(-0.5, -0.5, -0.5), float3(0.5, 0.5, 0.5)))
    {
        output_color = scene.cube_color;
    }

    RWTexture2D<float4> output_texture = get_rw_texture_2d_float4(push_constant.output_texture_descriptor_id);
    output_texture[texel_coord] = float4(output_color, 1.0f);
}