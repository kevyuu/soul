#pragma once

#include "core/type_traits.h"
#include "core/vec.h"
#include "math/quaternion.h"

namespace soul::math
{
  template <ts_floating_point T>
  struct Transform {
    Vec<T, 3> position;
    Vec<T, 3> scale;
    Quaternion<T> rotation;
  };

  inline namespace builtin
  {
    using xform32 = Transform<f32>;
    using xform64 = Transform<f64>;
  } // namespace builtin

  template <ts_floating_point T>
  auto into_transform(const mat4<T>& matrix) -> Transform<T>
  {
    Transform<T> result;
    result.position = Vec<T, 3>(matrix.col(3).xyz());

    const auto col0 = matrix.col(0);
    const auto col1 = matrix.col(1);
    const auto col2 = matrix.col(2);
    result.scale = Vec<T, 3>(
      length(Vec<T, 3>(col0.xyz())), length(Vec<T, 3>(col1.xyz())), length(Vec<T, 3>(col2.xyz())));

    float elem0 = col0.x / result.scale.x;
    float elem4 = col0.y / result.scale.x;
    float elem8 = col0.z / result.scale.x;

    float elem1 = col1.x / result.scale.y;
    float elem5 = col1.y / result.scale.y;
    float elem9 = col1.z / result.scale.y;

    float elem2 = col2.x / result.scale.z;
    float elem6 = col2.y / result.scale.z;
    float elem10 = col2.z / result.scale.z;

    float tr = elem0 + elem5 + elem10;
    if (tr > 0) {
      float w4 = sqrt(1.0f + tr) * 2.0f;
      result.rotation.w = 0.25f * w4;
      result.rotation.x = (elem9 - elem6) / w4;
      result.rotation.y = (elem2 - elem8) / w4;
      result.rotation.z = (elem4 - elem1) / w4;
    } else if ((elem0 > elem5) && (elem0 > elem10)) {
      float S = sqrt(1 + elem0 - elem5 - elem10) * 2;
      result.rotation.w = (elem9 - elem6) / S;
      result.rotation.x = 0.25f * S;
      result.rotation.y = (elem1 + elem4) / S;
      result.rotation.z = (elem2 + elem8) / S;
    } else if (elem5 > elem10) {
      float S = sqrt(1.0f + elem5 - elem0 - elem10) * 2;
      result.rotation.w = (elem2 - elem8) / S;
      result.rotation.x = (elem1 + elem4) / S;
      result.rotation.y = 0.25f * S;
      result.rotation.z = (elem6 + elem9) / S;
    } else {
      float S = sqrt(1.0f + elem10 - elem0 - elem5) * 2;
      result.rotation.w = (elem4 - elem1) / S;
      result.rotation.x = (elem2 + elem8) / S;
      result.rotation.y = (elem6 + elem9) / S;
      result.rotation.z = 0.25f * S;
    }
    result.rotation = normalize(result.rotation);
    return result;
  }

} // namespace soul::math
