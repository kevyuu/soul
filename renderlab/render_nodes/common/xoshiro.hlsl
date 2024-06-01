#ifndef SOULSL_XOSHIRO_HLSL
#define SOULSL_XOSHIRO_HLSL

struct Xoroshiro64StarRng
{
  vec2u32 state;
  
  void init(vec2u32 input)
  {
    state = input;
  }

  u32 rotl(u32 k)
  {
    u32 result = state.x * 0x9e3779bb;

    state.y ^= state.x;
    state.x = rng_rotl(state.x, 26) ^ state.y ^ (state.y << 9);
    state.y = rng_rotl(state.y, 13);

    return result;
  }

  u32 next()
  {
    u32 result = state.x * 0x9e3779bb;

    state.y ^= state.x;
    state.x = rng_rotl(26) ^ state.y ^ (state.y << 9);
    state.y = rng_rotl(13);

    return result;
  }

  f32 next_f32()
  {
    return asfloat(next());
  }

  vec2f32 next_vec2f32()
  {
    return vec2f32(next_f32(), next_f32());
  }

  vec3f32 next_vec3f32()
  {
    return vec3f32(next_f32(), next_f32(), next_f32());
  }
}

#endif
