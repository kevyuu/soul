#ifndef RENDERLAB_BND_SAMPLER_HLSL
#define RENDERLAB_BND_SAMPLER_HLSL

f32 sample_blue_noise(
  vec2i32 coord,
  i32 sample_index,
  i32 sample_dimension,
  Texture2D sobol_sequence_tex,
  Texture2D scrambling_ranking_tex)
{
  // wrap arguments
  coord.x          = coord.x % 128;
  coord.y          = coord.y % 128;
  sample_index     = sample_index % 256;
  sample_dimension = sample_dimension % 4;

  // xor index based on optimized ranking
  f32 rank                = scrambling_ranking_tex.Load(vec3i32(coord.x, coord.y, 0)).b;
  i32 ranked_sample_index = sample_index ^ i32(clamp(rank * 256.0f, 0.0f, 255.0f));

  // fetch value in sequence
  f32 sobol_sequence =
    sobol_sequence_tex.Load(vec3i32(ranked_sample_index, 0, 0))[sample_dimension];
  i32 value = i32(clamp(sobol_sequence * 256.0f, 0.0f, 255.0f));

  // If the dimension is optimized, xor sequence value based on optimized scrambling
  f32 scrambling = scrambling_ranking_tex.Load(vec3i32(coord.x, coord.y, 0))[sample_dimension % 2];
  value          = value ^ i32(clamp(scrambling * 256.0f, 0.0f, 255.0f));

  // convert to float and return
  float v = (0.5f + value) / 256.0f;
  return v;
}

#endif // RENDERLAB_BND_SAMPLER_HLSL
