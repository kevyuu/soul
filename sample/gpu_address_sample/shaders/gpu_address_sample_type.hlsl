#define WORK_GROUP_SIZE_X 16
#define WORK_GROUP_SIZE_Y 16

struct GPUScene
{
    soulsl::float3 sky_color;
    soulsl::float3 cube_color;
    soulsl::float4x4 projection_inverse;
    soulsl::float4x4 view_inverse;
};

struct PushConstant
{
    soulsl::uint2 dimension;
    soulsl::GPUAddress scene_gpu_address;
    soulsl::DescriptorID scene_descriptor_id;
    soulsl::DescriptorID output_texture_descriptor_id;
};