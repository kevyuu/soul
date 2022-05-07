#pragma once

#include "core/type.h"
#include "core/math.h"
#include "core/hash_map.h"

#include "data.h"

struct cgltf_data;
struct cgltf_node;
struct cgltf_texture;
struct cgltf_material;
struct cgltf_primitive;
struct cgltf_attribute;
struct cgltf_accessor;

namespace soul_fila
{
    enum UvSet : uint8_t { UNUSED, UV0, UV1 };
    constexpr int UV_MAP_SIZE = 8;
    using UvMap = std::array<UvSet, UV_MAP_SIZE>;

	class GLTFImporter
	{
	public:
        GLTFImporter(const char* gltf_path, soul::gpu::System& gpu_system, GPUProgramRegistry& program_registry, Scene& scene);
        void import();

	private:

        MaterialID create_material(const cgltf_material* src_material, const bool vertex_color, UvMap& uvmap);
        TextureID create_texture(const cgltf_texture& src_texture, const bool srgb);
        EntityID create_entity(const cgltf_node* node);
        void create_renderable(EntityID entity, const cgltf_node* node);
        void create_camera(EntityID entity_id, const cgltf_node* node);
        void create_light(EntityID entity_id, const cgltf_node* node);
        void import_textures();
        void import_entities();
        void import_animations();
        void import_skins();

        struct CGLTFNodeKey {
            const cgltf_node* node;

            explicit CGLTFNodeKey(const cgltf_node* node) : node(node) {}

            inline bool operator==(const CGLTFNodeKey& other) const {
                return (memcmp(this, &other, sizeof(CGLTFNodeKey)) == 0);
            }

            inline bool operator!=(const CGLTFNodeKey& other) const {
                return (memcmp(this, &other, sizeof(CGLTFNodeKey)) != 0);
            }
        };

        soul::HashMap<CGLTFNodeKey, EntityID> node_map_;

        struct TexCacheKey {
            const cgltf_texture* gltfTexture;
            bool srgb;

            inline bool operator==(const TexCacheKey& other) const {
                return (memcmp(this, &other, sizeof(TexCacheKey)) == 0);
            }

            inline bool operator!=(const TexCacheKey& other) const {
                return (memcmp(this, &other, sizeof(TexCacheKey)) != 0);
            }

        };
        using TexCache = soul::HashMap<TexCacheKey, TextureID>;
        TexCache tex_cache_;
        using TexCacheKeyList= soul::Array<TexCacheKey>;
        TexCacheKeyList tex_key_list_;

        struct MatCacheKey {
            intptr_t key;

            bool operator==(const MatCacheKey& other) const {
                return this->key == other.key;
            }

            bool operator!=(const MatCacheKey& other) const {
                return this->key != other.key;
            }

        };
        struct MatCacheEntry {
            MaterialID materialID;
            UvMap uvmap;
        };
        using MatCache = soul::HashMap<MatCacheKey, MatCacheEntry>;
        MatCache mat_cache_;

        const char* gltf_path_;
        soul::gpu::System& gpu_system_;
        GPUProgramRegistry& program_registry_;
        Scene& scene_;
        cgltf_data* asset_ = nullptr;
        
	};
}
