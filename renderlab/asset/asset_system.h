#include "core/builtins.h"
#include "core/hash_map.h"
#include "core/path.h"
#include "core/rid.h"

#include "type.h"

using namespace soul::builtin;

namespace renderlab
{
  using TextureAssetID  = u64;
  using MaterialAssetID = u64;
  using MeshAssetID     = u64;

  struct TextureAssetMetadata
  {
    TextureAssetID asset_id;
    soul::Path path;
    soul::u64 asset_reference_count;
    soul::gpu::TextureID texture_id;
  };

  struct StandardMaterialData
  {
    TextureAssetID base_color_texture;
    TextureAssetID metallic_roughness_texture;
    TextureAssetID normal_texture;
    TextureAssetID emissive_texture_id;

    vec4f32 base_color_factor;
    vec3f32 emissive_factor;
    f32 metallic_factor;
    f32 roughness_factor;
  };

  struct Material
  {
    MaterialAssetID asset_id;
    StandardMaterialData material_data;
  };

  struct MaterialAssetMetadata
  {
    MaterialAssetID asset_id;
    soul::Path path;
    soul::u64 asset_reference_count;
    MaterialID material_id;
  };

  struct MeshAssetMetadata
  {
    MeshAssetID asset_id;
    soul::Path path;
    soul::u64 asset_reference_count;
    MeshID mesh_id;
  };

  class AssetDatabase
  {
  public:
    void for_each_subdirectory(const Path& directory) {}

  private:
    soul::Pool<TextureAssetMetadata> texture_asset_metadata;
    soul::HashMap<TextureAssetID, u64> texture_asset_metadata_list;
    soul::HashMap<MaterialAssetID, u64> material_asset_metadata_list;
    soul::HashMap<MeshAssetID, u64> mesh_asset_list;
  };

} // namespace renderlab
