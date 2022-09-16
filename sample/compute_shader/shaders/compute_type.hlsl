#define WORK_GROUP_SIZE_X 16
#define WORK_GROUP_SIZE_Y 16

struct ComputePushConstant
{
    soulsl::DescriptorID output_uav_gpu_handle;
    soulsl::uint2 dimension;
    float t;
};
