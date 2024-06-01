#ifndef RENDERLAB_TAA_SHARED_HLSL
#define RENDERLAB_TAA_SHARED_HLSL

#define WORK_GROUP_SIZE_X 8
#define WORK_GROUP_SIZE_Y 8

struct TaaPC {
  soulsl::DescriptorID gpu_scene_buffer;

  soulsl::DescriptorID current_color_texture;
  soulsl::DescriptorID history_color_texture; 
  soulsl::DescriptorID motion_curve_gbuffer; 
  soulsl::DescriptorID depth_gbuffer;
  soulsl::DescriptorID output_texture;

  f32 feedback_min;
  f32 feedback_max;
  u32 sharpen_enable;
  u32 dilation_enable;
};

#endif
