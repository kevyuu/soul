#include "editor/intern/demo/demo.h"
#include "editor/intern/action.h"
#include "editor/intern/entity.h"
#include <time.h>
#include <stdlib.h>

namespace Soul {
	namespace Editor {

		void SeaOfLightDemo::init(Database* db) {

			const char* sponza_file_path = "assets/sponza/scene.gltf";
			const char* light_ball_file_path = "assets/first_try/scene.gltf";
			const int NUM_LIGHT_BALL = 100;
			sponza = ActionImportGLTFAsset(&db->world, sponza_file_path, true);

			srand(time(NULL));

			lightBalls.reserve(400);
			velocities.reserve(400);
			for (int i = 0; i < 400; i++)
			{
				EntityID id = ActionImportGLTFAsset(&db->world, light_ball_file_path, true);
				lightBalls.add(id);
				Vec3f position;
				position.x = ((float)(rand() % 3000) / 100) - 15.0f;
				position.y = ((float)(rand() % 1000) / 100);
				position.z = ((float)(rand() % 1000) / 100) - 5.0f;
				Entity* entity = EntityPtr(&db->world, id);
				MeshEntity* meshEntity = (MeshEntity*)entity;
				Material& material = db->world.materials[meshEntity->materialID];
				material.emissive.x = ((float)(rand() % 1000) / 1000);
				material.emissive.y = ((float)(rand() % 1000) / 1000);
				material.emissive.z = ((float)(rand() % 1000) / 1000);
				PoolArray<Texture>& textures = db->world.textures;
				Render::MaterialSpec spec = {
						textures[material.albedoTexID].rid,
						textures[material.normalTexID].rid,
						textures[material.metallicTexID].rid,
						textures[material.roughnessTexID].rid,
						textures[material.aoTexID].rid,
						textures[material.emissiveTexID].rid,

						material.useAlbedoTex,
						material.useNormalTex,
						material.useMetallicTex,
						material.useRoughnessTex,
						material.useAOTex,
						material.useEmissiveTex,

						material.albedo,
						material.metallic,
						material.roughness,
						material.emissive,

						material.metallicTextureChannel,
						material.roughnessTextureChannel,
						material.aoTextureChannel
				};
				db->world.renderSystem.materialUpdate(material.rid, spec);
				Transform worldTransform = entity->worldTransform;
				float scale = ((float)(rand() % 200) / 1000);
				worldTransform.scale.x *= scale;
				worldTransform.scale.y *= scale;
				worldTransform.scale.z *= scale;
				worldTransform.position = position;
				EntitySetWorldTransform(&db->world, EntityPtr(&db->world, id), worldTransform);
				velocities.add(((float)(rand() % 7) / 1000) + 0.01f);
			}

		}

		void SeaOfLightDemo::tick(Database* db) {
			for (int i = 0; i < 400; i++) {
				EntityID id = lightBalls[i];
				Entity* entity = EntityPtr(&db->world, id);
				Transform worldTransform = entity->worldTransform;
				worldTransform.position.y += velocities[i];
				if (worldTransform.position.y >= 10.0f) {
					worldTransform.position.y = -1.0f;
				}
				EntitySetWorldTransform(&db->world, entity, worldTransform);
			}
		}
		
		void SeaOfLightDemo::cleanup(Database* db) {

		}

	}
}