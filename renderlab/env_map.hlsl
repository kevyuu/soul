#ifndef RENDERLAB_ENV_MAP_HLSL
#define RENDERLAB_ENV_MAP_HLSL

#ifndef SOULSL_HOST_CODE
#  include "type.shared.hlsl"
#  include "utils/math.hlsl"

struct EnvMap
{
  Texture2D texture;
  SamplerState sampler;

  EnvMapSettingData setting_data;

  static EnvMap New(EnvMapData data, SamplerState sampler)
  {
    EnvMap result;
    result.texture = get_texture_2d(data.descriptor_id);
    result.sampler = sampler;
    result.setting_data = data.setting_data;
    return result;
  }

  vec3f32 sample_by_direction(vec3f32 direction)
  {
    vec3f32 local_direction = mul(setting_data.inv_transform, vec4f32(direction, 0)).xyz;
    const vec2f32 uv = equirectangular_uv_from_direction(local_direction);
    return texture.SampleLevel(sampler, uv, 0).rgb * setting_data.tint * setting_data.intensity;
  }
};
#endif // SOULSL_HOST_CODE

#endif
