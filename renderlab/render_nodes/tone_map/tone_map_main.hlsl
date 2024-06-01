#include "render_nodes/common/sv.hlsl"
#include "render_nodes/tone_map/tone_map.shared.hlsl"

vec3f32 aces_film(vec3f32 x)
{
  f32 a = 2.51f;
  f32 b = 0.03f;
  f32 c = 2.43f;
  f32 d = 0.59f;
  f32 e = 0.14f;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

vec3f32 reinhard(vec3f32 x)
{
  return x / (x + vec3f32(1, 1, 1));
}

[[vk::push_constant]] ToneMapPC push_constant;

[numthreads(WORK_GROUP_SIZE_X, WORK_GROUP_SIZE_Y, 1)] //
  void
  cs_main(ComputeSV sv)
{
  Texture2D input_texture             = get_texture_2d(push_constant.input_texture);
  RWTexture2D<vec4f32> output_texture = get_rw_texture_2d_vec4f32(push_constant.output_texture);

  const vec2i32 texel_id = sv.global_id.xy;
  const vec3i32 load_id  = vec3i32(texel_id, 0);

  vec3f32 output           = aces_film(input_texture.Load(load_id).xyz);
  f32 gamma                = 2.2;
  output                   = pow(output, 1.0f / gamma);
  output_texture[texel_id] = vec4f32(output, 1.0f);
}

// ------------------------------------------------------------------
