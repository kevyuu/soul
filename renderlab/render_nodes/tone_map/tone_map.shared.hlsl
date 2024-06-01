#ifndef RENDERLAB_TAA_SHARED_HLSL
#define RENDERLAB_TAA_SHARED_HLSL

#define WORK_GROUP_SIZE_X 8
#define WORK_GROUP_SIZE_Y 8

struct ToneMapPC {
  soulsl::DescriptorID input_texture;
  soulsl::DescriptorID output_texture;
};

#endif
