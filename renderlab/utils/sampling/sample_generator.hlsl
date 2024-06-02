#ifndef SOULSL_SAMPLING_SAMPLE_GENERATOR_HLSL
#define SOULSL_SAMPLING_SAMPLE_GENERATOR_HLSL

interface SampleGenerator
{
  vec2f32 sample_2d();
};

class BlueNoiseSampleGenerator : SampleGenerator
{

  Texture2D sobol_texture;
  Texture2D scrambling_ranking_texture;
  vec2i32 texel_id;
  i32 frame_index;

  static BlueNoiseSampleGenerator New(
    soulsl::DescriptorID sobol_descriptor_id,
    soulsl::DescriptorID scrambling_ranking_descriptor_id,
    vec2i32 texel_id,
    i32 frame_index)
  {
    BlueNoiseSampleGenerator result;
    result.sobol_texture              = get_texture_2d(sobol_descriptor_id);
    result.scrambling_ranking_texture = get_texture_2d(scrambling_ranking_descriptor_id);
    result.texel_id                   = texel_id;
    result.frame_index                = frame_index;
    return result;
  }

  f32 sample_blue_noise(i32 sample_dimension)
  {
    // wrap arguments
    texel_id.x       = texel_id.x % 128;
    texel_id.y       = texel_id.y % 128;
    frame_index      = frame_index % 256;
    sample_dimension = sample_dimension % 4;

    // xor index based on optimized ranking
    f32 rank                = scrambling_ranking_texture.Load(vec3i32(texel_id.x, texel_id.y, 0)).b;
    i32 ranked_sample_index = frame_index ^ i32(clamp(rank * 256.0f, 0.0f, 255.0f));

    // fetch value in sequence
    f32 sobol_sequence = sobol_texture.Load(vec3i32(ranked_sample_index, 0, 0))[sample_dimension];
    i32 value          = i32(clamp(sobol_sequence * 256.0f, 0.0f, 255.0f));

    // If the dimension is optimized, xor sequence value based on optimized scrambling
    f32 scrambling =
      scrambling_ranking_texture.Load(vec3i32(texel_id.x, texel_id.y, 0))[sample_dimension % 2];
    value = value ^ i32(clamp(scrambling * 256.0f, 0.0f, 255.0f));

    // convert to float and return
    float v = (0.5f + value) / 256.0f;
    return v;
  }

  vec2f32 sample_2d()
  {
    return vec2f32(sample_blue_noise(0), sample_blue_noise(1));
  }
};

#endif
