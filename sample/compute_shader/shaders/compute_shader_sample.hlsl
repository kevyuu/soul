#include "compute_type.hlsl"

[[vk::push_constant]]
ComputePushConstant push_constant;

[numthreads(WORK_GROUP_SIZE_X, WORK_GROUP_SIZE_Y, 1)]
void cs_main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    float4 value = float4(0.0, 0.0, 0.0, 1.0);
    int2 texel_coord = dispatch_thread_id.xy;

    float speed = 100;

    value.x = ((float(texel_coord.x) + push_constant.t * speed) % push_constant.dimension.x) / push_constant.dimension.x;
    value.y = float(texel_coord.y) / push_constant.dimension.y;

    RWTexture2D<float4> out_texture = get_rw_texture_2d_float4(push_constant.output_uav_gpu_handle);
    out_texture[texel_coord] = value;
}
