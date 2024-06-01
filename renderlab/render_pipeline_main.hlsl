#include "render_pipeline.shared.hlsl"

#include "render_nodes/common/sv.hlsl"

[[vk::push_constant]] RenderPipelinePC push_constant;

vec3f32 aces_film(vec3f32 x)
{
  f32 a = 2.51f;
  f32 b = 0.03f;
  f32 c = 2.43f;
  f32 d = 0.59f;
  f32 e = 0.14f;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

[numthreads(WORK_GROUP_SIZE_X, WORK_GROUP_SIZE_Y, 1)] //
  void
  cs_main(ComputeSV sv)
{
  const vec2i32 texel_coord = sv.global_id.xy;
  const vec3u32 load_index  = vec3u32(texel_coord, 0);

  Texture2D input_texture          = get_texture_2d(push_constant.input_texture);
  Texture2D overlay_texture          = get_texture_2d(push_constant.overlay_texture);
  RWTexture2D<vec4f32> out_texture = get_rw_texture_2d_vec4f32(push_constant.output_texture);

  vec4f32 input_val = input_texture.Load(load_index);
  vec4f32 overlay_val = overlay_texture.Load(load_index);

  vec4f32 color;
  for (u32 channel_i = 0; channel_i < 4; channel_i++)
  {
    switch (push_constant.value_options[channel_i])
    {
    case ValueOption::X:
    {
      color[channel_i] = input_val.x;
      break;
    }
    case ValueOption::Y:
    {
      color[channel_i] = input_val.y;
      break;
    }
    case ValueOption::Z:
    {
      color[channel_i] = input_val.z;
      break;
    }
    case ValueOption::W:
    {
      color[channel_i] = input_val.w;
      break;
    }
    case ValueOption::ONE:
    {
      color[channel_i] = 1.0f;
      break;
    }
    case ValueOption::ZERO:
    {
      color[channel_i] = 0.0f;
      break;
    }
    case ValueOption::COUNT:
    {
      color[channel_i] = 0.0f;
      break;
    }
    }
  }
  color.xyz = lerp(color.xyz, overlay_val.xyz, overlay_val.w);
  out_texture[texel_coord] = vec4f32(color.xyz, 1);
}
