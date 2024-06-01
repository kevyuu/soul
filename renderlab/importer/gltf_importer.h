#pragma once

#include "core/hash_map.h"
#include "core/not_null.h"
#include "core/path.h"
#include "gpu/type.h"

#include "ecs.h"
#include "type.h"

struct cgltf_node;
struct cgltf_data;
struct cgltf_attribute;
struct cgltf_mesh;
struct cgltf_primitive;
struct cgltf_material;
struct cgltf_texture;

using namespace soul;

namespace renderlab
{
  class Scene;

  class GLTFImporter
  {
  public:
    GLTFImporter() = default;
    GLTFImporter(const GLTFImporter& other) = delete;
    GLTFImporter(GLTFImporter&& other) = delete;
    auto operator=(const GLTFImporter& other) -> GLTFImporter& = delete;
    auto operator=(GLTFImporter&& other) -> GLTFImporter& = delete;
    ~GLTFImporter() = default;

    void import(const Path& path, NotNull<Scene*> scene);
    void cleanup();

  private:
    struct ImportContext {
      NotNull<const cgltf_data*> asset;
      NotNull<const Path*> path;
      NotNull<Scene*> scene;
    };
    struct TextureData {
      MaterialTextureID id;
      gpu::TextureFormat format = gpu::TextureFormat::COUNT;
    };
    using TextureMap = Vector<TextureData>;
    TextureMap texture_map_;

    using MaterialMap = Vector<MaterialID>;
    MaterialMap material_map_;

    using MeshGroupMap = Vector<MeshGroupID>;
    MeshGroupMap mesh_group_map_;

    using EntityMap = HashMap<const cgltf_node*, EntityId>;
    EntityMap entity_map_;

    auto get_or_create_material_texture(
      const cgltf_texture* src_texture, gpu::TextureFormat format, ImportContext import_context)
      -> MaterialTextureID;

    auto create_material(const cgltf_material& src_material, ImportContext import_context)
      -> MaterialID;

    auto get_mesh_group(const cgltf_mesh& src_mesh, ImportContext import_context) -> MeshGroupID;
    auto create_mesh_group(const cgltf_mesh& src_mesh, ImportContext import_context) -> MeshGroupID;

    auto create_entity(const cgltf_node& src_node, ImportContext import_context) -> EntityId;
  };
} // namespace renderlab
