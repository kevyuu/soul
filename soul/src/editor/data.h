#pragma once

#include "extern/ImGuizmo.h"

#include "core/pool_array.h"
#include "core/type.h"

#include "render/data.h"
#include "render/system.h"

struct GLFWwindow;

namespace Soul {

	namespace Editor {

		struct System;

		enum EntityType {

			EntityType_GROUP,
			EntityType_MESH,
			EntityType_DIRLIGHT,
			EntityType_POINTLIGHT,
			EntityType_SPOTLIGHT,

			EntityType_Count
		};

		struct EntityID {
			PoolID index;
			uint16 type;

			inline bool operator==(const EntityID& rhs) {
				return this->type == rhs.type && this->index == rhs.index;
			}

			inline bool operator!=(const EntityID& rhs) {
				return this->type != rhs.type || this->index != rhs.index;
			}

		};

		struct GroupEntity;

		struct Entity {
			static constexpr uint32 MAX_NAME_LENGTH = 1024;
			EntityID entityID;
			char name[MAX_NAME_LENGTH];

			GroupEntity* parent = nullptr;
			Entity* prev = nullptr;
			Entity* next = nullptr;

			Transform localTransform;
			Transform worldTransform;
		};

		struct GroupEntity : Entity {
			Entity* first;
		};

		struct DirLightEntity : Entity {
			Render::DirectionalLightSpec spec;
			Render::DirLightRID rid;
		};

		struct PointLightEntity : Entity {
			Render::PointLightSpec spec;
			Render::PointLightRID rid;
		};

		struct SpotLightEntity : Entity
		{
			Render::SpotLightSpec spec;
			Render::SpotLightRID rid;
		};

		struct MeshEntity : Entity {
			Render::MeshRID meshRID;
			uint32 materialID;
		};
		
		struct Material {
			char name[1024];
			Render::MaterialRID rid;

			uint32 albedoTexID;
			uint32 normalTexID;
			uint32 metallicTexID;
			uint32 roughnessTexID;
			uint32 aoTexID;

			Soul::Vec3f albedo;
			float metallic;
			float roughness;

			bool useAlbedoTex;
			bool useNormalTex;
			bool useMetallicTex;
			bool useRoughnessTex;
			bool useAOTex;

			Soul::Render::TexChannel metallicTextureChannel;
			Soul::Render::TexChannel roughnessTextureChannel;
			Soul::Render::TexChannel aoTextureChannel;

		};
		
		struct Texture {
			char name[1024];
			Render::TextureRID rid;
		};

		struct RenderConfig {
			struct EnvConfig {
				Vec3f ambientColor;
				float ambientEnergy;

				char panoramaFilePath[2048];
				Render::MaterialRID panoramaRID;
			} envConfig;
			Render::VoxelGIConfig voxelGIConfig;
			Render::ShadowAtlasConfig shadowAtlasConfig;
			Render::PostProcessConfig postProcessConfig;
		};

		struct World {

			EntityID rootEntityID;

			PoolArray<GroupEntity> groupEntities;
			PoolArray<MeshEntity> meshEntities;
			PoolArray<DirLightEntity> dirLightEntities;
			PoolArray<PointLightEntity> pointLightEntities;
			PoolArray<SpotLightEntity> spotLightEntities;

			PoolArray<Material> materials;
			PoolArray<Texture> textures;

			Render::Camera camera;
			RenderConfig renderConfig;
			Render::System renderSystem;

		};

		struct Database;

		struct MenuBar {
			bool setMeshPositionToAABBCenter = false;
			char gltfFilePath[1000] = {};
			void tick(Database* db);
		};

		struct EntityListPanel {
			void tick(Database* db);
		};

		struct EntityDetailPanel {
			void tick(Database* db);
		};

		struct RenderConfigPanel {
			void tick(Database* db);
		};

		struct Manipulator {
			ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
			ImGuizmo::MODE mode = ImGuizmo::WORLD;

			void tick(Database* db);
		};

		struct Database {

			World world;
			GLFWwindow* window;

			struct Widget {
				
				MenuBar menuBar;
				
				EntityListPanel entityListPanel;
				RenderConfigPanel renderConfigPanel;
				
				EntityDetailPanel entityDetailPanel;
				Manipulator manipulator;

			} widget;
			

			EntityID selectedEntity;
		};

	}

}