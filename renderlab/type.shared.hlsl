#ifndef RENDERLAB_TYPE_HLSL
#define RENDERLAB_TYPE_HLSL

enum class MeshInstanceFlag : u32
{
  USE_16_BIT_INDICES,
  COUNT,
};

struct MeshInstanceFlags
{
  u32 flags_;

#ifdef SOULSL_HOST_CODE
  constexpr MeshInstanceFlags(const std::initializer_list<MeshInstanceFlag> init_list) : flags_(0)
  {
    for (const auto val : init_list)
    {
      set(val);
    }
  }
#endif

  void set(MeshInstanceFlag flag, bool value = true)
  {
    flags_ |= (1U << (u32)(flag));
  }

  bool test(MeshInstanceFlag flag) SOULSL_CONST_FUNCTION
  {
    return flags_ & (1U << ((u32)flag));
  }
};

struct MeshInstance
{
  MeshInstanceFlags flags;
  u32 vb_offset;
  u32 ib_offset;
  u32 index_count;
  u32 mesh_id;
  u32 material_index;
  u32 matrix_index;
};

struct Material
{
  soulsl::DescriptorID base_color_texture_id;
  soulsl::DescriptorID metallic_roughness_texture_id;
  soulsl::DescriptorID normal_texture_id;
  soulsl::DescriptorID emissive_texture_id;

  vec4f32 base_color_factor;
  f32 metallic_factor;
  f32 roughness_factor;
  vec3f32 emissive_factor;
};

struct MaterialInstance
{
  vec3f32 albedo;
  f32 metallic;
  f32 roughness;
  vec3f32 normal;
  vec3f32 emissive;
};

struct StaticVertexData
{
  vec3f32 position;
  vec3f32 normal;
  vec4f32 tangent; ///< The tangent is *only* valid when tangent.w != 0.
  vec2f32 tex_coord;
};

struct VertexData
{
  vec3f32 world_position;
  vec3f32 world_normal;
  vec3f32 world_tangent;
  vec3f32 world_bitangent;
  vec2f32 tex_coord;
  u32 material_index;
};

struct GPUCameraData
{
  mat4f32 view_mat;
  mat4f32 proj_mat;
  mat4f32 proj_view_mat;
  mat4f32 proj_view_mat_no_jitter;
  mat4f32 inv_view_mat;
  mat4f32 inv_proj_mat;
  mat4f32 inv_proj_view_mat;

  vec3f32 position;
  vec3f32 target;
  vec3f32 up;
  f32 near_z;
  f32 far_z;
  vec3f32 camera_w;
  vec3f32 camera_u;
  vec3f32 camera_v;

  vec2f32 jitter;

#ifndef SOULSL_HOST_CODE
  vec3f32 compute_non_normalized_ray_dir_pinhole(vec2u32 pixel, vec2u32 frame_dim)
  {
    vec2f32 p   = (pixel + vec2f32(0.5f, 0.5f)) / frame_dim;
    vec2f32 ndc = vec2f32(2, -2) * p + vec2f32(-1, 1);

    return ndc.x * camera_u + ndc.y * camera_v + camera_w;
  }

  RayDesc compute_ray_pinhole(vec2u32 pixel, vec2u32 frame_dim)
  {
    RayDesc ray;

    ray.Origin    = position;
    ray.Direction = normalize(compute_non_normalized_ray_dir_pinhole(pixel, frame_dim));

    f32 inv_cos = 1.f / dot(normalize(camera_w), ray.Direction);
    ray.TMin    = near_z * inv_cos;
    ray.TMax    = far_z * inv_cos;

    return ray;
  }
#endif
};

enum class LightRadiationType : u32
{
  POINT,
  DIRECTIONAL,
  SPOT,
  COUNT,
};

struct GPULightInstance
{
  u32 radiation_type;
  vec3f32 position;
  vec3f32 direction;
  vec3f32 intensity;
  f32 cos_outer_angle;
  f32 cos_inner_angle;
};

struct MeshGeometryID
{
  u32 mesh_instance_id;
  u32 geometry_index; 

  static MeshGeometryID
  New(u32 mesh_instance_id, u32 geometry_index)
  {
    MeshGeometryID id;
    id.mesh_instance_id = mesh_instance_id;
    id.geometry_index = geometry_index;
    return id;
  }
};

struct EnvMapSettingData {
  mat4f32 transform; 
  mat4f32 inv_transform;
  vec3f32 tint;
  f32 intensity;
};

struct EnvMapData {
  soulsl::DescriptorID descriptor_id;
  EnvMapSettingData setting_data;
};

#endif // RENDERLAB_TYPE_HLSL
