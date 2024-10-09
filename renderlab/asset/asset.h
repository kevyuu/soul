
#include "core/path.h"
#include "core/rid.h"
#include "core/variant.h"
#include "core/vector.h"

#include "math/math.h"

#include "renderlab/type.shared.hlsl"
#include "type.shared.hlsl"

using namespace soul::builtin;

namespace renderlab
{
  using AssetHandle = u64;

  struct LightData
  {
    LightRadiationType type = LightRadiationType::COUNT;
    soul::vec3f32 color     = {1.0f, 1.0f, 1.0f};
    f32 intensity           = 1000.0f;
    f32 inner_angle         = 0.0f;
    f32 outer_angle         = soul::math::f32const::PI;
  };

  struct CameraData
  {
    f32 fovy;
    f32 near_z;
    f32 far_z;
    f32 aspect_ratio;
  };

  struct SubMeshData
  {
    MeshInstanceFlags flags = {};
    AssetHandle material;
  };

  struct MeshData
  {
    soul::Vector<SubMeshData> sub_meshes;
  };

  using EntityData = soul::Variant<LightData, CameraData, MeshData>;

  struct EntityAsset
  {
  };

  struct TextureAsset
  {
  };

  struct MaterialAsset
  {
  };

} // namespace renderlab
