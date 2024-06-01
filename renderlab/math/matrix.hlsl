#ifndef SOULSL_MATH_MATRIX_HLSL
#define SOULSL_MATH_MATRIX_HLSL

static const mat3f32 MAT3F32_IDENTITY = {vec3f32(1, 0, 0), vec3f32(0, 1, 0), vec3f32(0, 0, 1)};
static const mat4f32 MAT4F32_IDENTITY = {
  vec4f32(1, 0, 0, 0), vec4f32(0, 1, 0, 0), vec4f32(0, 0, 1, 0), vec4f32(0, 0, 0, 1)};

namespace math
{
  vec4f32 get_col(mat4f32 m, u32 idx)
  {
    return vec4f32(m[0][idx], m[1][idx], m[2][idx], m[3][idx]);
  }

  void set_col(inout mat4f32 m, u32 idx, vec4f32 v)
  {
    m[0][idx] = v.x;
    m[1][idx] = v.y;
    m[2][idx] = v.z;
    m[3][idx] = v.w;
  }

  mat4f32 mat4f32_from_columns(vec4f32 col0, vec4f32 col1, vec4f32 col2, vec4f32 col3)
  {
    mat4f32 result;
    set_col(result, 0, col0);
    set_col(result, 1, col1);
    set_col(result, 2, col2);
    set_col(result, 3, col3);
    return result;
  }

  mat4f32 translate(mat4f32 m, vec3f32 position)
  {
    mat4f32 result = m;
    vec4f32 col3   = get_col(m, 0) * position.x + get_col(m, 1) * position.y +
                   get_col(m, 2) * position.z + get_col(m, 3);
    set_col(result, 3, col3);
    return result;
  }

  mat4f32 translate_identity(vec3f32 position)
  {
    mat4f32 result = MAT4F32_IDENTITY;
    set_col(result, 3, vec4f32(position, 1));
    return result;
  }

  float4x4 scale(float4x4 m, vec3f32 scale)
  {
    mat4f32 result;
    set_col(result, 0, get_col(m, 0) * scale.x);
    set_col(result, 1, get_col(m, 1) * scale.y);
    set_col(result, 2, get_col(m, 2) * scale.z);
    set_col(result, 3, get_col(m, 3));
    return result;
  }

  mat4f32 scale_identity(vec3f32 scale)
  {
    mat4f32 result = MAT4F32_IDENTITY;
    result[0][0] = scale.x;
    result[1][1] = scale.y;
    result[2][2] = scale.z;
    return result;
  }

  mat4f32 orthonormal_basis(vec3f32 z)
  {
    const vec3f32 ref = abs(dot(z, vec3f32(0, 1, 0))) > 0.99f ? vec3f32(0, 0, 1) : vec3f32(0, 1, 0);

    const vec3f32 x = normalize(cross(ref, z));
    const vec3f32 y = cross(z, x);

    return mat4f32_from_columns(vec4f32(x, 0), vec4f32(y, 0), vec4f32(z, 0), vec4f32(0, 0, 0, 1));
  }

} // namespace math

#endif
