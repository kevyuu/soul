#pragma once

#include "core/type.h"
#include "editor/data.h"

namespace Soul {
	
	struct Render::DirectionalLightSpec;

	namespace Editor {

			Entity* EntityPtr(World* world, EntityID entityID);
			EntityID EntityCreate(World* world, EntityID parent, EntityType entityType, const char* name, Transform localTransform);
			void EntityDelete(World* world, EntityID entityID);
			void EntityDelete(World* world, Entity* entity);
			void EntitySetLocalTransform(World* world, Entity* entity, const Transform& localTransform);
			void EntitySetWorldTransform(World* world, Entity* entity, const Transform& worldTransform);

			void GroupEntityCreate(World* world, GroupEntity* groupEntity, const char* name, Transform localTransform);
			void GroupEntityDelete(World* world, GroupEntity* groupEntity);
			void GroupEntitySetLocalTransform(World* world, GroupEntity* entity, const Transform& localTransform);
			void GroupEntitySetWorldTransform(World* world, GroupEntity* entity, const Transform& worldTransform);
			
			void MeshEntityCreate(World* world, MeshEntity* meshEntity, const char* name, Transform localTransform);
			void MeshEntityDelete(World* world, MeshEntity* meshEntity);
			void MeshEntitySetLocalTransform(World* world, MeshEntity* meshEntity, const Transform& localTransform);
			void MeshEntitySetWorldTransform(World* world, MeshEntity* meshEntity, const Transform& worldTransform);

			EntityID DirLightEntityCreate(World* world, GroupEntity* parent, const char* name, Transform& localTransform, const Render::DirectionalLightSpec& spec);
			void DirLightEntityDelete(World* world, DirLightEntity* dirLightEntity);
			void DirLightEntitySetDirection(World* world, DirLightEntity* dirLightEntity, const Vec3f& direction);
			void DirLightEntitySetLocalTransform(World* world, DirLightEntity* dirLightEntity, const Transform& localTransform);
			void DirLightEntitySetWorldTransform(World* world, DirLightEntity* dirLightEntity, const Transform& worldTransform);

			EntityID SpotLightEntityCreate(World* world, GroupEntity* parent, const char* name, Transform& localTransform, const Render::SpotLightSpec& spec);
			void SpotLightEntityDelete(World* world, SpotLightEntity* spotLightEntity);
			void SpotLightEntitySetDirection(World* world, SpotLightEntity* spotLightEntity, const Vec3f& direction);
			void SpotLightEntitySetLocalTransform(World* world, SpotLightEntity* spotLightEntity, const Transform& localTransform);
			void SpotLightEntitySetWorldTransform(World* world, SpotLightEntity* spotLightEntity, const Transform& worldTransform);

			EntityID PointLightEntityCreate(World* world, GroupEntity* parent, const char* name, Transform& localTransform, const Render::PointLightSpec& spec);
			void PointLightEntityDelete(World* world, PointLightEntity* pointLightEntity);
			void PointLightEntitySetLocalTransform(World* world, PointLightEntity* pointLightEntity, const Transform& localTransform);
			void PointLightEntitySetWorldTransform(World* world, PointLightEntity* pointLightEntity, const Transform& worldTransform);
	}
}