#ifndef RENDERLAB_RENDER_PIPELINE_SHARED_HLSL
#define RENDERLAB_RENDER_PIPELINE_SHARED_HLSL

enum class PostProcessOption : u32 {
  NONE,
  ACES,
  UNPACK_32UI,
  COUNT
};

enum class ValueOption : u32 {
  X,
  Y,
  Z,
  W,
  ONE,
  ZERO,
  COUNT
};

#define WORK_GROUP_SIZE_X 8
#define WORK_GROUP_SIZE_Y 8

struct RenderPipelinePC {
  soulsl::DescriptorID input_texture;
  soulsl::DescriptorID overlay_texture;
  soulsl::DescriptorID output_texture;
  PostProcessOption postprocess_option;
  ValueOption value_options[4];
};

#endif
