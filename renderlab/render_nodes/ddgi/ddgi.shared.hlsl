#ifndef RENDERLAB_DDGI_SHARED_HLSL
#define RENDERLAB_DDGI_SHARED_HLSL

#ifndef SOULSL_HOST_CODE
#  include "utils/math.hlsl"
#endif

#define PROBE_OCT_SIZE             16
#define PROBE_OCT_SIZE_WITH_BORDER 18
#define GAMMA_IRRADIANCE           5.0f

struct DdgiVolume
{
  vec3f32 grid_start_position;
  f32 dummy;
  vec3f32 grid_step;
  f32 dummy2;
  vec3i32 probe_counts;
  f32 dummy3;
  f32 max_depth;
  f32 depth_sharpness;
  f32 hysteresis;
  f32 crush_threshold;
  f32 max_delta_luma;
  f32 sample_normal_bias;
  f32 visibility_normal_bias;
  f32 bounce_intensity;
  f32 energy_preservation;
  i32 probe_map_texture_width;
  i32 probe_map_texture_height;
  i32 rays_per_probe;

#ifndef SOULSL_HOST_CODE
  vec3i32 base_grid_coord_from_position(vec3f32 X)
  {
    return clamp(
      vec3i32((X - grid_start_position) / grid_step),
      vec3i32(0, 0, 0),
      vec3i32(probe_counts) - vec3i32(1, 1, 1));
  }

  vec3f32 position_from_grid_coord(vec3i32 c)
  {
    return grid_step * vec3f32(c) + grid_start_position;
  }

  vec3f32 position_from_probe_index(i32 probe_index)
  {
    return position_from_grid_coord(grid_coord_from_probe_index(probe_index));
  }

  i32 probe_index_from_grid_coord(vec3i32 probe_coords)
  {
    return i32(
      probe_coords.x + probe_coords.y * probe_counts.x +
      probe_coords.z * probe_counts.x * probe_counts.y);
  }

  i32 probe_index_from_probe_map_coord(vec2i32 texel_id)
  {
    const i32 probe_with_border_size = PROBE_OCT_SIZE + 2;
    const i32 probes_per_side        = (probe_map_texture_width - 2) / probe_with_border_size;
    return i32(texel_id.x / probe_with_border_size) +
           probes_per_side * i32(texel_id.y / probe_with_border_size);
  }

  vec3i32 grid_coord_from_probe_index(i32 index)
  {
    vec3i32 i_pos;

    // Slow, but works for any # of probes
    i_pos.x = index % probe_counts.x;
    i_pos.y = (index % (probe_counts.x * probe_counts.y)) / probe_counts.x;
    i_pos.z = index / (probe_counts.x * probe_counts.y);

    // Assumes probeCounts are powers of two.
    // Saves ~10ms compared to the divisions above
    // Precomputing the MSB actually slows this code down substantially
    //    i_pos.x = index & (probe_counts.x - 1);
    //    i_pos.y = (index & ((probe_counts.x * probe_counts.y) - 1)) >>
    //    findMSB(probe_counts.x); i_pos.z = index >> findMSB(probe_counts.x *
    //    probe_counts.y);

    return i_pos;
  }

  vec3f32 probe_location_from_probe_index(i32 index)
  {
    vec3i32 grid_coord = grid_coord_from_probe_index(index);

    return position_from_grid_coord(grid_coord);
  }

  vec2f32 oct_snorm_from_probe_map_texel_id(vec2i32 texel_id)
  {
    const i32 probe_with_border_size = PROBE_OCT_SIZE + 2;

    const vec2f32 oct_texel_id =
      vec2i32((texel_id.x - 2) % probe_with_border_size, (texel_id.y - 2) % probe_with_border_size);

    // Add back the half pixel to get pixel center normalized coordinates
    return (vec2f32(oct_texel_id) + vec2f32(0.5f, 0.5f)) * (2.0f / f32(PROBE_OCT_SIZE)) -
           vec2f32(1.0f, 1.0f);
  }

  vec2f32 texture_coord_from_direction(vec3f32 dir, i32 probe_index)
  {
    vec2f32 oct_unorm = oct_unorm_from_ndir(dir);

    // Length of a probe side, plus one pixel on each edge for the border
    f32 probe_with_border_size = f32(PROBE_OCT_SIZE_WITH_BORDER);

    vec2f32 probe_map_dimension    = vec2f32(probe_map_texture_width, probe_map_texture_height);
    i32 probes_per_row             = probe_counts.x * probe_counts.y;
    vec2f32 probe_map_texel_offset = oct_unorm * f32(PROBE_OCT_SIZE);

    vec2f32 probe_map_base_texel = vec2f32(
                                     (probe_index % probes_per_row) * probe_with_border_size,
                                     (probe_index / probes_per_row) * probe_with_border_size) +
                                   vec2f32(2.0f, 2.0f);

    return (probe_map_base_texel + probe_map_texel_offset) / probe_map_dimension;
  }

  vec3f32 get_sampling_direction(mat3f32 rotation_mat, u32 ray_id)
  {
    const vec3f32 sample = spherical_fibonacci(ray_id, rays_per_probe);
    return normalize(mul(rotation_mat, sample));
  }

  vec3f32 sample_irradiance(
    vec3f32 position,
    vec3f32 normal,
    vec3f32 wo,
    SamplerState sampler,
    Texture2D irradiance_probe_texture,
    Texture2D depth_probe_texture)
  {
    vec3i32 base_grid_coord = base_grid_coord_from_position(position);
    vec3f32 base_probe_pos  = position_from_grid_coord(base_grid_coord);

    vec3f32 sum_unweighted_irradiance = 0.0f;
    vec3f32 sum_irradiance            = 0.0f;
    f32 sum_weight                    = 0.0f;

    // alpha is how far from the floor(currentVertex) position. on [0, 1] for each axis.
    vec3f32 alpha = clamp((position - base_probe_pos) / grid_step, 0.0f, 1.0f);

    // Iterate over adjacent probe cage
    for (i32 i = 0; i < 8; ++i)
    {
      // Compute the offset grid coord and clamp to the probe grid boundary
      // Offset = 0 or 1 along each axis
      vec3i32 offset           = vec3i32(i, i >> 1, i >> 2) & vec3i32(1, 1, 1);
      vec3i32 probe_grid_coord = clamp(base_grid_coord + offset, 0, probe_counts - 1);
      i32 probe_index          = probe_index_from_grid_coord(probe_grid_coord);

      // Make cosine falloff in tangent plane with respect to the angle from the surface to the
      // probe so that we never test a probe that is *behind* the surface. It doesn't have to be
      // cosine, but that is efficient to compute and we must clip to the tangent plane.
      vec3f32 probe_pos = position_from_grid_coord(probe_grid_coord);

      // Bias the position at which visibility is computed; this
      // avoids performing a shadow test *at* a surface, which is a
      // dangerous location because that is exactly the line between
      // shadowed and unshadowed. If the normal bias is too small,
      // there will be light and dark leaks. If it is too large,
      // then samples can pass through thin occluders to the other
      // side (this can only happen if there are MULTIPLE occluders
      // near each other, a wall surface won't pass through itself.)

      f32 min_axial_distance = min(grid_step.x, min(grid_step.y, grid_step.z));
      vec3f32 probe_to_point =
        position - probe_pos +
        (normal * 0.2 + 0.8 * wo) * (0.75 * min_axial_distance) * sample_normal_bias;
      vec3f32 dir = normalize(-probe_to_point);

      // Compute the trilinear weights based on the grid cell vertex to smoothly
      // transition between probes. Avoid ever going entirely to zero because that
      // will cause problems at the border probes. This isn't really a lerp.
      // We're using 1-a when offset = 0 and a when offset = 1.
      vec3f32 trilinear = lerp(1.0 - alpha, alpha, offset);
      f32 weight        = 1.0;

      // Clamp all of the multiplies. We can't let the weight go to zero because then it would be
      // possible for *all* weights to be equally low and get normalized
      // up to 1/n. We want to distinguish between weights that are
      // low because of different factors.

      // Smooth backface test
      {
        // Computed without the biasing applied to the "dir" variable.
        // This test can cause reflection-map looking errors in the image
        // (stuff looks shiny) if the transition is poor.
        vec3f32 true_direction_to_probe = normalize(probe_pos - position);

        // The naive soft backface weight would ignore a probe when
        // it is behind the surface. That's good for walls. But for small details inside of a
        // room, the normals on the details might rule out all of the probes that have mutual
        // visibility to the point. So, we instead use a "wrap shading" test below inspired by
        // NPR work.
        // weight *= max(0.0001, dot(true_direction_to_probe, normal));

        // The small offset at the end reduces the "going to zero" impact
        // where this is really close to exactly opposite
        weight *= square((dot(true_direction_to_probe, normal) + 1.0)) + 0.05;
      }

      vec2f32 depth_probe_tex_coord = texture_coord_from_direction(-dir, probe_index);

      f32 dist_to_probe = length(probe_to_point);

      vec2f32 moment = depth_probe_texture.SampleLevel(sampler, depth_probe_tex_coord, 0).rg;
      f32 mean       = moment.x;
      f32 variance   = abs(square(moment.x) - moment.y);

      // http://www.punkuser.net/vsm/vsm_paper.pdf; equation 5
      // Need the max in the denominator because biasing can cause a negative displacement
      f32 chebyshev_weight = variance / (variance + square(max(dist_to_probe - mean, 0.0)));

      // Increase contrast in the weight
      chebyshev_weight = max(pow3(chebyshev_weight), 0.0);

      weight *= chebyshev_weight;

      // weight *= luminance(irradiance_probe_texture.SampleLevel(sampler, depth_probe_tex_coord, 0).rgb);

      // Avoid zero weight
      weight = max(0.0001, weight);


      vec3f32 irradiance_dir = normal;

      vec2f32 irradiance_probe_tex_coord =
        texture_coord_from_direction(normalize(irradiance_dir), probe_index);

      vec3f32 probe_irradiance =
        irradiance_probe_texture.SampleLevel(sampler, irradiance_probe_tex_coord, 0).rgb;

      // Decode the tone curve, but leave a gamma = 2 curve
      // to approximate sRGB blending for the trilinear
      probe_irradiance = pow(probe_irradiance, GAMMA_IRRADIANCE * 0.5);

      // A tiny bit of light is really visible due to log perception, so
      // crush tiny weights but keep the curve continuous. This must be done
      // before the trilinear weights, because those should be preserved.
      if (weight < crush_threshold)
      {
        weight *= weight * weight * (1.0f / square(crush_threshold));
      }

      // Trilinear weights
      weight *= trilinear.x * trilinear.y * trilinear.z;

      sum_unweighted_irradiance += probe_irradiance;
      sum_irradiance += weight * probe_irradiance;
      sum_weight += weight;
    }

    vec3f32 net_irradiance = sum_irradiance / sum_weight;

    net_irradiance.x = isnan(net_irradiance.x) ? 0.5f : net_irradiance.x;
    net_irradiance.y = isnan(net_irradiance.y) ? 0.5f : net_irradiance.y;
    net_irradiance.z = isnan(net_irradiance.z) ? 0.5f : net_irradiance.z;

    net_irradiance = square(net_irradiance);

    // Go back to linear irradiance
    net_irradiance *= energy_preservation;

    return 0.5f * M_PI * net_irradiance;
  }
#endif
};

struct RayTracingPC
{
  mat4f32 rotation;
  DdgiVolume ddgi_volume;
  u32 frame_idx;

  soulsl::DescriptorID gpu_scene_id;
  soulsl::DescriptorID irradiance_output_texture;
  soulsl::DescriptorID direction_depth_output_texture;
  soulsl::DescriptorID history_irradiance_probe_texture;
  soulsl::DescriptorID history_depth_probe_texture;
};

#define PROBE_UPDATE_WORK_GROUP_SIZE_X PROBE_OCT_SIZE
#define PROBE_UPDATE_WORK_GROUP_SIZE_Y PROBE_OCT_SIZE

struct ProbeUpdatePC
{
  DdgiVolume ddgi_volume;
  u32 frame_counter;
  soulsl::DescriptorID ray_radiance_texture;
  soulsl::DescriptorID ray_dir_dist_texture;
  soulsl::DescriptorID history_irradiance_probe_texture;
  soulsl::DescriptorID history_depth_probe_texture;

  // output texture
  soulsl::DescriptorID irradiance_probe_texture;
  soulsl::DescriptorID depth_probe_texture;
};

#define PROBE_BORDER_UPDATE_WORK_GROUP_SIZE_X 8
#define PROBE_BORDER_UPDATE_WORK_GROUP_SIZE_Y 8
#define PROBE_BORDER_UPDATE_WORK_GROUP_COUNT  64

struct ProbeBorderUpdatePC
{
  soulsl::DescriptorID irradiance_probe_texture;
  soulsl::DescriptorID depth_probe_texture;
};

#define SAMPLE_IRRADIANCE_WORK_GROUP_SIZE_X 8
#define SAMPLE_IRRADIANCE_WORK_GROUP_SIZE_Y 8

struct SampleIrradiancePC
{
  DdgiVolume ddgi_volume;
  soulsl::DescriptorID gpu_scene_id;
  soulsl::DescriptorID depth_texture;
  soulsl::DescriptorID normal_roughness_texture;
  soulsl::DescriptorID irradiance_probe_texture;
  soulsl::DescriptorID depth_probe_texture;
  soulsl::DescriptorID output_texture;
};

struct ProbeOverlayPC
{
  DdgiVolume ddgi_volume;
  f32 probe_radius;

  soulsl::DescriptorID gpu_scene_id;
  soulsl::DescriptorID irradiance_probe_texture;
  soulsl::DescriptorID depth_probe_texture;
};

struct RayOverlayPC
{
  DdgiVolume ddgi_volume;
  u32 probe_index;

  soulsl::DescriptorID gpu_scene_id;
  soulsl::DescriptorID irradiance_texture;
  soulsl::DescriptorID ray_dir_dist_texture;
};

#endif // RENDERLAB_DDGI_SHARED_HLSL
