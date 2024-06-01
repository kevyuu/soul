#ifndef SOUL_REPROJECTION_HLSL
#define SOUL_REPROJECTION_HLSL

#define NORMAL_DISTANCE 0.1f
#define PLANE_DISTANCE  5.0f

#include "utils/math.hlsl"

b8 plane_distance_disocclusion_check(
  vec3f32 current_pos, vec3f32 history_pos, vec3f32 current_normal)
{
  const vec3f32 to_current = current_pos - history_pos;
  const f32 dist_to_plane  = abs(dot(to_current, current_normal));

  return dist_to_plane > PLANE_DISTANCE;
}

b8 out_of_frame_disocclusion_check(vec2i32 coord, vec2i32 image_dim)
{
  if (any(coord < vec2i32(0, 0)) || any(coord > image_dim - vec2i32(1, 1)))
  {
    return true;
  } else
  {
    return false;
  }
}

b8 mesh_id_disocclusion_check(f32 mesh_id, f32 mesh_id_prev)
{
  if (mesh_id == mesh_id_prev)
  {
    return false;
  } else
  {
    return true;
  }
}

b8 normals_disocclusion_check(vec3f32 current_normal, vec3f32 history_normal)
{
  if (pow(abs(dot(current_normal, history_normal)), 2) > NORMAL_DISTANCE)
  {
    return false;
  } else
  {
    return true;
  }
}

b8 is_reprojection_valid(
  vec2i32 coord,
  vec3f32 current_pos,
  vec3f32 history_pos,
  vec3f32 current_normal,
  vec3f32 history_normal,
  f32 current_mesh_id,
  f32 history_mesh_id,
  vec2i32 image_dim)
{
  if (out_of_frame_disocclusion_check(coord, image_dim))
  {
    return false;
  }

  if (mesh_id_disocclusion_check(current_mesh_id, history_mesh_id))
  {
    return false;
  }

  if (plane_distance_disocclusion_check(current_pos, history_pos, current_normal))
  {
    return false;
  }

  if (normals_disocclusion_check(current_normal, history_normal))
  {
    return false;
  }

  return true;
}

// ------------------------------------------------------------------

vec2f32 surface_point_reprojection(vec2i32 coord, vec2f32 motion_vector, vec2i32 size)
{
  return vec2f32(coord) + motion_vector.xy * vec2f32(size);
}

// ------------------------------------------------------------------

vec2f32 virtual_point_reprojection(
  vec2i32 current_coord,
  vec2i32 size,
  f32 depth,
  f32 ray_length,
  vec3f32 cam_pos,
  mat4f32 inv_proj_view,
  mat4f32 prev_proj_view)
{
  const vec2f32 tex_coord  = current_coord / vec2f32(size);
  const vec3f32 ray_origin = world_position_from_depth(tex_coord, depth, inv_proj_view);

  vec3f32 camera_ray = ray_origin - cam_pos.xyz;

  const f32 camera_ray_length     = length(camera_ray);
  const f32 reflection_ray_length = ray_length;

  camera_ray = normalize(camera_ray);

  const vec3f32 parallax_hit_point =
    cam_pos.xyz + camera_ray * (camera_ray_length + reflection_ray_length);

  vec4f32 reprojected_parallax_hit_point = mul(prev_proj_view, vec4f32(parallax_hit_point, 1.0f));

  reprojected_parallax_hit_point.xy /= reprojected_parallax_hit_point.w;

  return (reprojected_parallax_hit_point.xy * 0.5f + 0.5f) * vec2f32(size);
}

// ------------------------------------------------------------------------

vec2f32 compute_history_coord(
  vec2i32 current_coord,
  vec2i32 size,
  f32 depth,
  vec2f32 motion,
  f32 curvature,
  f32 ray_length,
  vec3f32 cam_pos,
  mat4f32 inv_proj_view,
  mat4f32 prev_proj_view)
{
  const vec2f32 surface_history_coord = surface_point_reprojection(current_coord, motion, size);

  vec2f32 history_coord = surface_history_coord;

  if (ray_length > 0.0f && curvature == 0.0f)
  {
    history_coord = virtual_point_reprojection(
      current_coord, size, depth, ray_length, cam_pos, inv_proj_view, prev_proj_view);
  }

  return history_coord;
}
#endif // SOUL_REPROJECTION_HLSL
