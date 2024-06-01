#include "render_nodes/common/color.hlsl"
#include "render_nodes/common/misc.hlsl"
#include "render_nodes/common/sv.hlsl"
#include "render_nodes/taa/taa.shared.hlsl"

#include "scene.hlsl"

[[vk::push_constant]] TaaPC push_constant;

vec3f32 find_closest_fragment_3x3(vec2f32 uv, SamplerState sampler, vec2f32 texel_uv_size)
{
  Texture2D depth_texture = get_texture_2d(push_constant.depth_gbuffer);

  vec2f32 dd = abs(texel_uv_size);
  vec2f32 du = vec2f32(dd.x, 0.0);
  vec2f32 dv = vec2f32(0.0, dd.y);

  vec3f32 dtl = vec3f32(-1, -1, depth_texture.SampleLevel(sampler, uv - dv - du, 0).x);
  vec3f32 dtc = vec3f32(0, -1, depth_texture.SampleLevel(sampler, uv - dv, 0).x);
  vec3f32 dtr = vec3f32(1, -1, depth_texture.SampleLevel(sampler, uv - dv + du, 0).x);

  vec3f32 dml = vec3f32(-1, 0, depth_texture.SampleLevel(sampler, uv - du, 0).x);
  vec3f32 dmc = vec3f32(0, 0, depth_texture.SampleLevel(sampler, uv, 0).x);
  vec3f32 dmr = vec3f32(1, 0, depth_texture.SampleLevel(sampler, uv + du, 0).x);

  vec3f32 dbl = vec3f32(-1, 1, depth_texture.SampleLevel(sampler, uv + dv - du, 0).x);
  vec3f32 dbc = vec3f32(0, 1, depth_texture.SampleLevel(sampler, uv + dv, 0).x);
  vec3f32 dbr = vec3f32(1, 1, depth_texture.SampleLevel(sampler, uv + dv + du, 0).x);

  vec3f32 dmin = dtl;
  if (dmin.z > dtc.z)
  {
    dmin = dtc;
  }
  if (dmin.z > dtr.z)
  {
    dmin = dtr;
  }

  if (dmin.z > dml.z)
  {
    dmin = dml;
  }
  if (dmin.z > dmc.z)
  {
    dmin = dmc;
  }
  if (dmin.z > dmr.z)
  {
    dmin = dmr;
  }

  if (dmin.z > dbl.z)
  {
    dmin = dbl;
  }
  if (dmin.z > dbc.z)
  {
    dmin = dbc;
  }
  if (dmin.z > dbr.z)
  {
    dmin = dbr;
  }

  return vec3f32(uv + dd.xy * dmin.xy, dmin.z);
}

vec4f32 sample_color(Texture2D texture, SamplerState sampler, vec2f32 uv)
{
  vec4f32 color = texture.SampleLevel(sampler, uv, 0);
  return vec4f32(RGB_to_YCoCg(color.rgb), color.a);
}

vec3f32 resolve_color(vec3f32 color)
{
  return YCoCg_to_RGB(color);
}

vec3f32 tonemap(vec3f32 x)
{
  return x / (x + vec3f32(1.0f, 1.0f, 1.0f));
}

vec3f32 inverse_tonemap(vec3f32 x)
{
  vec3f32 divisor =
    max(vec3f32(1.0f, 1.0f, 1.0f) - x, vec3f32(FLT_EPSILON, FLT_EPSILON, FLT_EPSILON));
  return x / divisor;
}

vec3f32 temporal_integrate(
  vec2f32 uv, vec2f32 jitter, vec2f32 motion, f32 depth, SamplerState sampler, vec2f32 texel_uv_size)
{
  Texture2D current_color_texture = get_texture_2d(push_constant.current_color_texture);
  Texture2D history_color_texture = get_texture_2d(push_constant.history_color_texture);

  const vec2f32 current_uv = uv + jitter;
  const vec2f32 history_uv = uv + motion;

  vec4f32 color         = sample_color(current_color_texture, sampler, current_uv);
  vec4f32 history_color = sample_color(history_color_texture, sampler, history_uv);

  vec2f32 du  = vec2f32(texel_uv_size.x, 0.0);
  vec2f32 dv  = vec2f32(0.0, texel_uv_size.y);
  vec4f32 ctl = sample_color(current_color_texture, sampler, current_uv - dv - du);
  vec4f32 ctc = sample_color(current_color_texture, sampler, current_uv - dv);
  vec4f32 ctr = sample_color(current_color_texture, sampler, current_uv - dv + du);
  vec4f32 cml = sample_color(current_color_texture, sampler, current_uv - du);
  vec4f32 cmc = sample_color(current_color_texture, sampler, current_uv);
  vec4f32 cmr = sample_color(current_color_texture, sampler, current_uv + du);
  vec4f32 cbl = sample_color(current_color_texture, sampler, current_uv + dv - du);
  vec4f32 cbc = sample_color(current_color_texture, sampler, current_uv + dv);
  vec4f32 cbr = sample_color(current_color_texture, sampler, current_uv + dv + du);
  vec4f32 cmin =
    min(ctl, min(ctc, min(ctr, min(cml, min(cmc, min(cmr, min(cbl, min(cbc, cbr))))))));
  vec4f32 cmax =
    max(ctl, max(ctc, max(ctr, max(cml, max(cmc, max(cmr, max(cbl, max(cbc, cbr))))))));

  vec4f32 cavg = (ctl + ctc + ctr + cml + cmc + cmr + cbl + cbc + cbr) / 9.0;

  history_color.xyz = clip_aabb(cmin.xyz, cmax.xyz, history_color.xyz);

  const f32 current_luma = color.r;
  const f32 history_luma = history_color.r;

  const f32 unbiased_diff =
    abs(current_luma - history_luma) / max(current_luma, max(history_luma, 0.2));
  const f32 unbiased_weight     = 1.0 - unbiased_diff;
  const f32 unbiased_weight_sqr = unbiased_weight * unbiased_weight;
  const f32 k_feedback =
    lerp(push_constant.feedback_min, push_constant.feedback_max, unbiased_weight_sqr);

  color.xyz         = resolve_color(color.xyz);
  history_color.xyz = resolve_color(history_color.xyz);

  color.xyz         = tonemap(color.xyz);
  history_color.xyz = tonemap(history_color.xyz);

  const vec3f32 ldr_output = mix(color.xyz, history_color.xyz, k_feedback);
  const vec3f32 hdr_output = inverse_tonemap(ldr_output);
  return hdr_output;
}

[numthreads(WORK_GROUP_SIZE_X, WORK_GROUP_SIZE_Y, 1)] //
  void
  cs_main(ComputeSV sv)
{
  const GPUScene scene = get_buffer<GPUScene>(push_constant.gpu_scene_buffer, 0);

  Texture2D depth_texture        = get_texture_2d(push_constant.depth_gbuffer);
  Texture2D motion_curve_gbuffer = get_texture_2d(push_constant.motion_curve_gbuffer);

  RWTexture2D<vec4f32> output_texture = get_rw_texture_2d_vec4f32(push_constant.output_texture);

  const vec2i32 texel_id = sv.global_id.xy;
  const vec3i32 load_id  = vec3i32(texel_id, 0);

  const vec2f32 image_dim     = vec2f32(get_texture_dimension(motion_curve_gbuffer));
  const vec2f32 texel_uv_size = vec2f32(1.0f, 1.0f) / image_dim;
  const vec2f32 pixel_center  = vec2f32(texel_id) + vec2f32(0.5f, 0.5f);
  const vec2f32 uv            = pixel_center / vec2f32(image_dim);
  const vec2f32 unjittered_uv = uv + scene.camera_data.jitter;

  SamplerState sampler = scene.get_linear_clamp_sampler();

  vec2f32 motion;
  f32 depth;
  if (push_constant.dilation_enable != 0)
  {
    vec3f32 closest_fragment = find_closest_fragment_3x3(unjittered_uv, sampler, texel_uv_size);
    motion                   = motion_curve_gbuffer.SampleLevel(sampler, closest_fragment.xy, 0).xy;
    depth                    = closest_fragment.z;
  } else
  {
    motion = motion_curve_gbuffer.SampleLevel(sampler, unjittered_uv, 0).xy;
    depth  = depth_texture.SampleLevel(sampler, unjittered_uv, 0).x;
  }
  vec2f32 unjittered_motion = motion + scene.camera_data.jitter - scene.prev_camera_data.jitter;

  vec3f32 output = temporal_integrate(uv, scene.camera_data.jitter, unjittered_motion, depth, sampler, texel_uv_size);

  if (any(isnan(output)))
  {
    output = vec3f32(0.5f, 0.5f, 0.5f);
  }
  output_texture[texel_id] = vec4f32(output, 1.0f);
}

// ------------------------------------------------------------------
