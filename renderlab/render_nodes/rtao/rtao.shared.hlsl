#define RAY_QUERY_WORK_GROUP_SIZE_X 8
#define RAY_QUERY_WORK_GROUP_SIZE_Y 4

struct RayQueryPC
{
  soulsl::DescriptorID gpu_scene_id;
  soulsl::DescriptorID normal_roughness_texture;
  soulsl::DescriptorID depth_texture;
  soulsl::DescriptorID output_texture;
  soulsl::DescriptorID sobol_texture;
  soulsl::DescriptorID scrambling_ranking_texture;
  f32 bias;
  f32 ray_min;
  f32 ray_max;
  u64 num_frames;
};

struct InitDispatchArgsPC
{
  soulsl::DescriptorID filter_dispatch_arg_buffer;
  soulsl::DescriptorID copy_dispatch_arg_buffer;
};

#define TEMPORAL_ACCUMULATION_WORK_GROUP_SIZE_X 8
#define TEMPORAL_ACCUMULATION_WORK_GROUP_SIZE_Y 8

struct TemporalAccumulationPC {
  soulsl::DescriptorID gpu_scene_id;

  // Current GBuffers
  soulsl::DescriptorID current_normal_roughness_gbuffer;
  soulsl::DescriptorID current_motion_curve_gbuffer;
  soulsl::DescriptorID current_meshid_gbuffer;
  soulsl::DescriptorID current_depth_gbuffer;

  // Prev GBuffers
  soulsl::DescriptorID prev_normal_roughness_gbuffer;
  soulsl::DescriptorID prev_motion_curve_gbuffer;
  soulsl::DescriptorID prev_meshid_gbuffer;
  soulsl::DescriptorID prev_depth_gbuffer;

  soulsl::DescriptorID ray_query_result_texture;

  soulsl::DescriptorID output_val_texture;
  soulsl::DescriptorID output_history_length_texture;

  soulsl::DescriptorID prev_val_texture;
  soulsl::DescriptorID prev_history_length_texture;

  soulsl::DescriptorID filter_dispatch_arg_buffer;
  soulsl::DescriptorID filter_coords_buffer;

  f32 alpha;
};

#define BILATERAL_BLUR_WORK_GROUP_SIZE_X 8
#define BILATERAL_BLUR_WORK_GROUP_SIZE_Y 8

struct BilateralBlurPC
{
  soulsl::DescriptorID filter_coords_buffer;
  soulsl::DescriptorID gbuffer_normal_roughness;
  soulsl::DescriptorID gbuffer_depth;
  soulsl::DescriptorID ao_input_texture;

  soulsl::DescriptorID output_texture;

  i32 radius; 
  vec2i32 direction;
};

struct DispatchArg
{
  u32 x;
  u32 y;
  u32 z;
};
