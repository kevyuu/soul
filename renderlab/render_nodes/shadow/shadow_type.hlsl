#define RAY_QUERY_WORK_GROUP_SIZE_X 8
#define RAY_QUERY_WORK_GROUP_SIZE_Y 4

struct ShadowPushConstant
{
  soulsl::DescriptorID gpu_scene_id;
  soulsl::DescriptorID normal_roughness_texture;
  soulsl::DescriptorID depth_texture;
  soulsl::DescriptorID output_texture;
  soulsl::DescriptorID sobol_texture;
  soulsl::DescriptorID scrambling_ranking_texture;
  u64 num_frames;
};

struct InitDispatchArgsPC
{
  soulsl::DescriptorID filter_dispatch_arg_buffer;
  soulsl::DescriptorID copy_dispatch_arg_buffer;
};

#define TEMPORAL_DENOISE_WORK_GROUP_SIZE_X 8
#define TEMPORAL_DENOISE_WORK_GROUP_SIZE_Y 8

struct TemporalDenoisePC
{
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
  soulsl::DescriptorID output_moment_length_texture;

  soulsl::DescriptorID prev_val_texture;
  soulsl::DescriptorID prev_moment_length_texture;

  soulsl::DescriptorID filter_dispatch_arg_buffer;
  soulsl::DescriptorID copy_dispatch_arg_buffer;
  soulsl::DescriptorID filter_coords_buffer;
  soulsl::DescriptorID copy_coords_buffer;

  f32 alpha;
  f32 moments_alpha;
};

struct DispatchArg
{
  u32 x;
  u32 y;
  u32 z;
};

#define COPY_TILE_WORK_GROUP_SIZE_X 8
#define COPY_TILE_WORK_GROUP_SIZE_Y 8

struct CopyTilePC
{
  soulsl::DescriptorID output_texture;
  soulsl::DescriptorID copy_coords_buffer;
};

#define FILTER_TILE_WORK_GROUP_SIZE_X 8
#define FILTER_TILE_WORK_GROUP_SIZE_Y 8

struct FilterTilePC
{
  soulsl::DescriptorID output_texture;

  soulsl::DescriptorID filter_coords_buffer;
  soulsl::DescriptorID visibility_texture;
  soulsl::DescriptorID gbuffer_normal_roughness;
  soulsl::DescriptorID gbuffer_depth;

  i32 radius;
  i32 step_size;
  f32 phi_visibility;
  f32 phi_normal;
  f32 sigma_depth;
  f32 power;
};
