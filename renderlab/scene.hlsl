#ifndef RENDERLAB_SCENE_HLSL
#define RENDERLAB_SCENE_HLSL

#include "type.shared.hlsl"

#include "env_map.hlsl"

struct GPUScene
{

  soulsl::DescriptorID world_matrixes_buffer;
  soulsl::DescriptorID normal_matrixes_buffer;
  soulsl::DescriptorID prev_world_matrixes_buffer;
  soulsl::DescriptorID prev_normal_matrixes_buffer;

  soulsl::DescriptorID mesh_instance_buffer;

  soulsl::DescriptorID vertices;
  soulsl::DescriptorID indexes;

  soulsl::DescriptorID material_buffer;
  EnvMapData env_map_data;

  soulsl::DescriptorID linear_repeat_sampler;
  soulsl::DescriptorID linear_clamp_sampler;
  soulsl::DescriptorID nearest_clamp_sampler;

  soulsl::DescriptorID tlas;

  GPUCameraData camera_data;
  GPUCameraData prev_camera_data;

  u64 light_count;
  soulsl::DescriptorID light_instance_buffer;

#ifndef SOULSL_HOST_CODE
  MeshInstance get_mesh_instance(u32 instance_index)
  {
    return get_buffer_array<MeshInstance>(mesh_instance_buffer, instance_index);
  }

  MeshInstance get_mesh_instance(MeshGeometryID mesh_geometry_id)
  {
    return get_mesh_instance(mesh_geometry_id.mesh_instance_id + mesh_geometry_id.geometry_index);
  }

  mat4f32 get_world_matrix(u32 matrix_index)
  {
    return get_buffer_array<mat4f32>(world_matrixes_buffer, matrix_index);
  }

  mat4f32 get_normal_matrix(u32 matrix_index)
  {
    return get_buffer_array<mat4f32>(normal_matrixes_buffer, matrix_index);
  }

  mat4f32 get_prev_world_matrix(u32 matrix_index)
  {
    return get_buffer_array<mat4f32>(prev_world_matrixes_buffer, matrix_index);
  }

  mat4f32 get_prev_normal_matrix(u32 matrix_index)
  {
    return get_buffer_array<mat4f32>(prev_normal_matrixes_buffer, matrix_index);
  }

  Material get_material(u32 material_index)
  {
    return get_buffer_array<Material>(material_buffer, material_index);
  }

  MaterialInstance get_material_instance(u32 material_index, vec2f32 tex_coord, mat3f32 tbn_matrix)
  {
    Material material = get_material(material_index);

    vec4f32 base_color = material.base_color_factor;
    if (material.base_color_texture_id.is_valid())
    {
      Texture2D base_color_texture    = get_texture_2d(material.base_color_texture_id);
      SamplerState base_color_sampler = get_linear_repeat_sampler();
      base_color =
        base_color * base_color_texture.SampleLevel(base_color_sampler, tex_coord, 0).xyzw;
    }

    vec2f32 metallic_roughness = vec2f32(material.metallic_factor, material.roughness_factor);
    if (material.metallic_roughness_texture_id.is_valid())
    {
      Texture2D metallic_roughness_texture = get_texture_2d(material.metallic_roughness_texture_id);
      SamplerState metallic_roughness_sampler = get_linear_repeat_sampler();
      metallic_roughness =
        metallic_roughness *
        metallic_roughness_texture.SampleLevel(metallic_roughness_sampler, tex_coord, 0).xy;
    }

    vec3f32 normal = normalize(tbn_matrix._m02_m12_m22);
    if (material.normal_texture_id.is_valid())
    {
      Texture2D normal_texture    = get_texture_2d(material.normal_texture_id);
      SamplerState normal_sampler = get_linear_repeat_sampler();
      normal                      = normal_texture.SampleLevel(normal_sampler, tex_coord, 0).xyz;
      normal                      = normal * 2.0 - 1.0;
      normal                      = normalize(mul(tbn_matrix, normal));
    }

    vec3f32 emissive = material.emissive_factor;
    if (material.emissive_texture_id.is_valid())
    {
      Texture2D emissive_texture    = get_texture_2d(material.emissive_texture_id);
      SamplerState emissive_sampler = get_linear_repeat_sampler();
      emissive = emissive * emissive_texture.SampleLevel(emissive_sampler, tex_coord, 0).xyz;
    }

    MaterialInstance result;
    result.albedo    = base_color.xyz;
    result.metallic  = metallic_roughness.x;
    result.roughness = metallic_roughness.y;
    result.normal    = normal;
    result.emissive  = emissive;

    return result;
  }

  MaterialInstance get_material_instance_ps(
    u32 material_index, vec2f32 tex_coord, mat3f32 tbn_matrix)
  {
    Material material = get_material(material_index);

    vec4f32 base_color = material.base_color_factor;
    if (material.base_color_texture_id.is_valid())
    {
      Texture2D base_color_texture    = get_texture_2d(material.base_color_texture_id);
      SamplerState base_color_sampler = get_linear_repeat_sampler();
      base_color = base_color * base_color_texture.Sample(base_color_sampler, tex_coord).xyzw;
    }

    vec2f32 metallic_roughness = vec2f32(material.metallic_factor, material.roughness_factor);
    if (material.metallic_roughness_texture_id.is_valid())
    {
      Texture2D metallic_roughness_texture = get_texture_2d(material.metallic_roughness_texture_id);
      SamplerState metallic_roughness_sampler = get_linear_repeat_sampler();
      metallic_roughness =
        metallic_roughness *
        metallic_roughness_texture.Sample(metallic_roughness_sampler, tex_coord).xy;
    }

    vec3f32 normal = tbn_matrix._m02_m12_m22;
    if (material.normal_texture_id.is_valid())
    {
      Texture2D normal_texture    = get_texture_2d(material.normal_texture_id);
      SamplerState normal_sampler = get_linear_repeat_sampler();
      normal                      = normal_texture.Sample(normal_sampler, tex_coord).xyz;
      normal                      = normal * 2.0 - 1.0;
      normal                      = normalize(mul(tbn_matrix, normal));
    }

    vec3f32 emissive = material.emissive_factor;
    if (material.emissive_texture_id.is_valid())
    {
      Texture2D emissive_texture    = get_texture_2d(material.emissive_texture_id);
      SamplerState emissive_sampler = get_linear_repeat_sampler();
      emissive = emissive * emissive_texture.Sample(emissive_sampler, tex_coord).xyz;
    }

    MaterialInstance result;
    result.albedo    = base_color.xyz;
    result.metallic  = metallic_roughness.x;
    result.roughness = metallic_roughness.y;
    result.normal    = normal;
    result.emissive  = emissive;

    return result;
  }

  SamplerState get_linear_clamp_sampler()
  {
    return get_sampler(linear_clamp_sampler);
  }

  SamplerState get_linear_repeat_sampler()
  {
    return get_sampler(linear_repeat_sampler);
  }

  SamplerState get_nearest_clamp_sampler()
  {
    return get_sampler(nearest_clamp_sampler);
  }

  GPULightInstance get_light_instance(u32 instance_index)
  {
    return get_buffer_array<GPULightInstance>(light_instance_buffer, instance_index);
  }

  StaticVertexData get_static_vertex_data(u32 index)
  {
    return get_buffer_array<StaticVertexData>(vertices, index);
  }

  vec3u32 get_local_indices(u32 ib_offset, u32 triangle_index, b8 use_16_bit)
  {
    u32 base_index = ib_offset * 4;
    vec3u32 vtx_indices;
    if (use_16_bit)
    {
      base_index += triangle_index * 6;
      if (base_index % 4 == 0)
      {
        u32 load1     = get_buffer<u32>(indexes, base_index);
        u32 load2     = get_buffer<u16>(indexes, base_index + 4);
        vtx_indices.x = load1 & 0xFFFF;
        vtx_indices.y = load1 >> 16;
        vtx_indices.z = load2;
      } else
      {
        u32 load1     = get_buffer<u32>(indexes, base_index - 2);
        u32 load2     = get_buffer<u32>(indexes, base_index + 2);
        vtx_indices.x = load1 >> 16;
        vtx_indices.y = load2 & 0xFFFF;
        vtx_indices.z = load2 >> 16;
      }
    } else
    {
      base_index += triangle_index * 12;
      vtx_indices = get_buffer<vec3u32>(indexes, base_index);
    }
    return vtx_indices;
  }

  vec3u32 get_indices(MeshGeometryID mesh_geometry_id, u32 triangle_index)
  {
    const MeshInstance mesh_instance = get_mesh_instance(mesh_geometry_id);
    vec3u32 vtx_indices              = get_local_indices(
      mesh_instance.ib_offset,
      triangle_index,
      mesh_instance.flags.test(MeshInstanceFlag::USE_16_BIT_INDICES));
    vtx_indices += mesh_instance.vb_offset;
    return vtx_indices;
  }

  VertexData get_vertex_data(
    MeshGeometryID mesh_geometry_id, u32 triangle_index, vec3f32 barycentrics)
  {
    const MeshInstance mesh_instance = get_mesh_instance(mesh_geometry_id);
    vec3u32 vtx_indices              = get_indices(mesh_geometry_id, triangle_index);

    StaticVertexData static_vertex_data[3] = {
      get_static_vertex_data(vtx_indices.x),
      get_static_vertex_data(vtx_indices.y),
      get_static_vertex_data(vtx_indices.z),
    };
    mat4f32 world_matrix  = get_world_matrix(mesh_instance.matrix_index);
    mat4f32 normal_matrix = get_normal_matrix(mesh_instance.matrix_index);

    const vec3f32 position = static_vertex_data[0].position * barycentrics[0] +
                             static_vertex_data[1].position * barycentrics[1] +
                             static_vertex_data[2].position * barycentrics[2];

    const vec3f32 normal = static_vertex_data[0].normal * barycentrics[0] +
                           static_vertex_data[1].normal * barycentrics[1] +
                           static_vertex_data[2].normal * barycentrics[2];

    const vec4f32 tangent = static_vertex_data[0].tangent * barycentrics[0] +
                            static_vertex_data[1].tangent * barycentrics[1] +
                            static_vertex_data[2].tangent * barycentrics[2];

    const vec2f32 tex_coord = static_vertex_data[0].tex_coord * barycentrics[0] +
                              static_vertex_data[1].tex_coord * barycentrics[1] +
                              static_vertex_data[2].tex_coord * barycentrics[2];

    VertexData result;
    result.world_position  = mul(world_matrix, vec4f32(position, 1)).xyz;
    result.world_normal    = mul(normal_matrix, vec4f32(normal, 0.0)).xyz;
    result.world_tangent   = mul(normal_matrix, vec4f32(tangent.xyz, 0.0)).xyz;
    result.world_bitangent = cross(result.world_normal, result.world_tangent) * tangent.w;
    result.tex_coord       = tex_coord;
    result.material_index  = mesh_instance.material_index;

    return result;
  }

  RaytracingAccelerationStructure get_tlas()
  {
    return get_as(tlas);
  }

  EnvMap get_env_map()
  {
    return EnvMap::New(env_map_data, get_linear_repeat_sampler());
  }

#endif // SOUL_HOST_CODE
};

#endif // RENDERLAB_SCENE_HLSL
