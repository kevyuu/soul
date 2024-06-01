#ifndef RENDERLAB_RT_REFLECTION_SHARED_HLSL
#define RENDERLAB_RT_REFLECTION_SHARED_HLSL

#define MIRROR_REFLECTIONS_ROUGHNESS_THRESHOLD 0.05f
#define DDGI_REFLECTIONS_ROUGHNESS_THRESHOLD 0.75f

struct RayTracePC
{
  soulsl::DescriptorID gpu_scene_id;
  soulsl::DescriptorID sobol_texture;
  soulsl::DescriptorID scrambling_ranking_texture;
  soulsl::DescriptorID depth_gbuffer;
  soulsl::DescriptorID normal_roughness_gbuffer;

  soulsl::DescriptorID output_texture;

  u32 num_frames;
  f32 trace_normal_bias;
  f32 lobe_trim;
};

struct InitDispatchArgsPC
{
  soulsl::DescriptorID filter_dispatch_arg_buffer;
  soulsl::DescriptorID copy_dispatch_arg_buffer;
};

struct DispatchArg
{
  u32 x;
  u32 y;
  u32 z;
};

#define TEMPORAL_ACCUMULATION_WORK_GROUP_SIZE_X 8
#define TEMPORAL_ACCUMULATION_WORK_GROUP_SIZE_Y 8

struct TemporalAccumulationPC
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

  soulsl::DescriptorID ray_trace_result_texture;

  soulsl::DescriptorID output_color_variance_texture;
  soulsl::DescriptorID output_moments_texture;

  soulsl::DescriptorID prev_color_variance_texture;
  soulsl::DescriptorID prev_moments_texture;

  soulsl::DescriptorID filter_dispatch_arg_buffer;
  soulsl::DescriptorID copy_dispatch_arg_buffer;
  soulsl::DescriptorID filter_coords_buffer;
  soulsl::DescriptorID copy_coords_buffer;

  f32 alpha;
  f32 moments_alpha;
};

#define COPY_TILE_WORK_GROUP_SIZE_X 8
#define COPY_TILE_WORK_GROUP_SIZE_Y 8

struct CopyTilePC
{
  soulsl::DescriptorID input_texture;
  soulsl::DescriptorID output_texture;
  soulsl::DescriptorID copy_coords_buffer;
};

#define FILTER_TILE_WORK_GROUP_SIZE_X 8
#define FILTER_TILE_WORK_GROUP_SIZE_Y 8

struct FilterTilePC
{
  soulsl::DescriptorID output_texture;

  soulsl::DescriptorID filter_coords_buffer;
  soulsl::DescriptorID color_texture;
  soulsl::DescriptorID gbuffer_normal_roughness;
  soulsl::DescriptorID gbuffer_depth;

  i32 radius;
  i32 step_size;
  f32 phi_color;
  f32 phi_normal;
  f32 sigma_depth;
};
#endif
