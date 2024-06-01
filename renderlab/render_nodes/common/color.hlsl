#ifndef SOULSL_COLOR_HLSL
#define SOULSL_COLOR_HLSL

inline vec3f32 RGB_to_YCoCg(vec3f32 c)
{
  return vec3f32(
    c.x / 4.0 + c.y / 2.0 + c.z / 4.0, c.x / 2.0 - c.z / 2.0, -c.x / 4.0 + c.y / 2.0 - c.z / 4.0);
}

inline vec3f32 YCoCg_to_RGB(vec3f32 c)
{
  f32 tmp = c.x - c.z;
  f32 r = tmp + c.y;
  f32 g = c.x + c.z;
  f32 b = tmp - c.y;
  return vec3f32(r, g, b);
}

#endif
