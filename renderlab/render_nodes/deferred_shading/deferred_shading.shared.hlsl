#ifndef RENDERLAB_DEFERRED_SHADING_SHARED_HLSL
#define RENDERLAB_DEFERRED_SHADING_SHARED_HLSL

#define WORK_GROUP_SIZE_X 8
#define WORK_GROUP_SIZE_Y 8

struct DeferredShadingPC
{
  soulsl::DescriptorID gpu_scene_id;
  soulsl::DescriptorID light_visibility_texture;
  soulsl::DescriptorID ao_texture;
  soulsl::DescriptorID albedo_metallic_texture;
  soulsl::DescriptorID motion_curve_texture;
  soulsl::DescriptorID normal_roughness_texture; 
  soulsl::DescriptorID depth_texture;
  soulsl::DescriptorID indirect_diffuse_texture;
  soulsl::DescriptorID indirect_specular_texture;
  soulsl::DescriptorID brdf_lut_texture;
  soulsl::DescriptorID output_texture;
  f32 indirect_diffuse_intensity;
  f32 indirect_specular_intensity;
};

#endif // RENDERLAB_DEFERRED_SHADING_SHARED_HLSL
