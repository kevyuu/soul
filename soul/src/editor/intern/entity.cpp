#include "core/debug.h"
#include "core/math.h"

#include "editor/data.h"
#include "editor/intern/entity.h"

#include "render/data.h"

namespace Soul {
	namespace Editor {

		Entity* EntityPtr(World* world, EntityID entityID) {
			switch (entityID.type) {
			case EntityType_MESH:
				return world->meshEntities.ptr(entityID.index);
			case EntityType_GROUP:
				return world->groupEntities.ptr(entityID.index);
			case EntityType_DIRLIGHT:
				return world->dirLightEntities.ptr(entityID.index);
			default:
				SOUL_ASSERT(0, false, "Entity type is not valid, entity type = %d", entityID.type);
			}
		}

		EntityID EntityCreate(World* world, EntityID parentID, EntityType entityType, const char* name, Transform localTransform) {
			EntityID entityID;
			entityID.type = entityType;
			Entity* entity = nullptr;
			switch (entityType) {
			case EntityType_MESH:
				entityID.index = world->meshEntities.add(MeshEntity());
				entity = world->meshEntities.ptr(entityID.index);
				break;
			case EntityType_GROUP:
				entityID.index = world->groupEntities.add(GroupEntity());
				entity = world->groupEntities.ptr(entityID.index);
				break;
			case EntityType_DIRLIGHT:
				entityID.index = world->dirLightEntities.add(DirLightEntity());
				entity = world->dirLightEntities.ptr(entityID.index);
			default:
				SOUL_ASSERT(0, false, "Entity type is not valid, entity type = %d", entityType);
			}

			GroupEntity* parent = (GroupEntity*) EntityPtr(world, parentID);
			Entity* next = parent->first;
			if (next != nullptr) {
				next->prev = entity;
			}

			entity->entityID = entityID;
			entity->next = next;
			entity->prev = nullptr;
			entity->parent = parent;
			parent->first = entity;

			entity->localTransform = localTransform;
			entity->worldTransform = parent->worldTransform * localTransform;

			SOUL_ASSERT(0, strlen(name) <= Entity::MAX_NAME_LENGTH, "Entity name exceed max length. Name = %s", name);
			strcpy(entity->name, name);

			return entityID;
		}

		void EntityDelete(World* world, EntityID entityID) {
			Entity* entity = EntityPtr(world, entityID);
			EntityDelete(world, entity);
		}

		void EntityDelete(World* world, Entity* entity) {
			switch (entity->entityID.type) {
			case EntityType_MESH:
				MeshEntityDelete(world, (MeshEntity*)entity);
				break;
			case EntityType_GROUP:
				GroupEntityDelete(world, (GroupEntity*)entity);
				break;
			case EntityType_DIRLIGHT:
				DirLightEntityDelete(world, (DirLightEntity*)entity);
				break;
			default:
				SOUL_ASSERT(0, false, "Entity type is invalid. Entity type = %d", entity->entityID.type);
			}
		}

		void EntitySetLocalTransform(World* world, Entity* entity, const Transform& localTransform) {
			switch (entity->entityID.type) {
			case EntityType_MESH:
				MeshEntitySetLocalTransform(world, (MeshEntity*)entity, localTransform);
				break;
			case EntityType_GROUP:
				GroupEntitySetLocalTransform(world, (GroupEntity*)entity, localTransform);
				break;
			case EntityType_DIRLIGHT:
				DirLightEntitySetLocalTransform(world, (DirLightEntity*)entity, localTransform);
				break;
			default:
				SOUL_ASSERT(0, false, "Entity type is invalid | Entity type = %d", entity->entityID.type);
				break;
			}
		}

		void EntitySetWorldTransform(World* world, Entity* entity, const Transform& worldTransform) {
			switch (entity->entityID.type) {
			case EntityType_MESH:
				MeshEntitySetWorldTransform(world, (MeshEntity*)entity, worldTransform);
				break;
			case EntityType_GROUP:
				GroupEntitySetWorldTransform(world, (GroupEntity*)entity, worldTransform);
				break;
			case EntityType_DIRLIGHT:
				DirLightEntitySetWorldTransform(world, (DirLightEntity*)entity, worldTransform);
				break;
			default:
				SOUL_ASSERT(0, false, "Entity type is invalid | Entity type = %d", entity->entityID.type);
				break;
			}
		}

		void _CommonEntityDelete(Entity* entity) {
			GroupEntity* parent = entity->parent;
			Entity* prev = entity->prev;
			Entity* next = entity->next;

			if (prev == nullptr) {
				parent->first = next;
			}
			else {
				prev->next = next;
			}

			if (next != nullptr) {
				next->prev = prev;
			}
		}

		void _CommonEntitySetLocalTransform(Entity* entity, const Transform& localTransform) {
			entity->localTransform = localTransform;
			entity->worldTransform = entity->parent->worldTransform * localTransform;
		}

		void _CommonEntitySetWorldTransform(Entity* entity, const Transform& worldTransform) {
			entity->worldTransform = worldTransform;
			Mat4 localMat4 = mat4Inverse(mat4Transform(entity->parent->worldTransform)) * mat4Transform(worldTransform);
			entity->localTransform = transformMat4(localMat4);
		}

		void GroupEntitySetLocalTransform(World* world, GroupEntity* groupEntity, const Transform& localTransform) {
			_CommonEntitySetLocalTransform((Entity*)groupEntity, localTransform);
			Entity* child = groupEntity->first;
			while (child != nullptr) {
				EntitySetWorldTransform(world, child, groupEntity->worldTransform * child->localTransform);
				child = child->next;
			}
		}

		void GroupEntitySetWorldTransform(World* world, GroupEntity* groupEntity, const Transform& worldTransform) {
			_CommonEntitySetWorldTransform((Entity*)groupEntity, worldTransform);
			Entity* child = groupEntity->first;
			while (child != nullptr) {
				EntitySetWorldTransform(world, child, groupEntity->worldTransform * child->localTransform);
				child = child->next;
			}
		}

		void GroupEntityDelete(World* world, GroupEntity* groupEntity) {
			_CommonEntityDelete((Entity*)groupEntity);
			while (groupEntity->first != nullptr) {
				EntityDelete(world, groupEntity->first);
			}
			world->groupEntities.remove(groupEntity->entityID.index);
		}

		void MeshEntitySetLocalTransform(World* world, MeshEntity* meshEntity, const Transform& localTransform) {
			_CommonEntitySetLocalTransform((Entity*)meshEntity, localTransform);
			world->renderSystem.meshSetTransform(meshEntity->meshRID, meshEntity->worldTransform);
		}

		void MeshEntitySetWorldTransform(World* world, MeshEntity* meshEntity, const Transform& worldTransform) {
			_CommonEntitySetWorldTransform((Entity*)meshEntity, worldTransform);
			world->renderSystem.meshSetTransform(meshEntity->meshRID, meshEntity->worldTransform);
		}

		void MeshEntityDelete(World* world, MeshEntity* meshEntity) {
			_CommonEntityDelete((Entity*)meshEntity);
			world->renderSystem.meshDestroy(meshEntity->meshRID);
			world->meshEntities.remove(meshEntity->entityID.index);
		}

		EntityID DirLightEntityCreate(World* world, GroupEntity* parent, const char* name, Transform& localTransform, const Render::DirectionalLightSpec& spec) {
			EntityID entityID;
			entityID.type = EntityType_DIRLIGHT;
			Render::MaterialRID lightRID = world->renderSystem.dirLightCreate(spec);
			DirLightEntity dirLightEntity;
			dirLightEntity.spec = spec;
			dirLightEntity.lightRID = lightRID;
			entityID.index = world->dirLightEntities.add(dirLightEntity);

			Entity* entity = (Entity*)world->dirLightEntities.ptr(entityID.index);

			Entity* next = parent->first;
			if (next != nullptr) {
				next->prev = entity;
			}

			entity->entityID = entityID;
			entity->next = next;
			entity->prev = nullptr;
			entity->parent = parent;
			parent->first = entity;

			entity->worldTransform = entity->parent->worldTransform * localTransform;
			entity->worldTransform = parent->worldTransform * localTransform;
			entity->worldTransform.rotation = quaternionFromVec3f(Vec3f(0.0f, 0.0f, 1.0f), spec.direction);
			Mat4 localMat4 = mat4Inverse(mat4Transform(entity->parent->worldTransform)) * mat4Transform(entity->worldTransform);
			entity->localTransform = transformMat4(localMat4);

			SOUL_ASSERT(0, strlen(name) <= Entity::MAX_NAME_LENGTH, "Entity name exceed max length. Name = %s", name);
			strcpy(entity->name, name);

			return entityID;
		}

		void DirLightEntityDelete(World* world, DirLightEntity* dirLightEntity) {
			_CommonEntityDelete((Entity*)dirLightEntity);
			world->renderSystem.dirLightDestroy(dirLightEntity->lightRID);
			world->dirLightEntities.remove(dirLightEntity->entityID.index);
		}

		void DirLightEntitySetDirection(World* world, DirLightEntity* dirLightEntity, const Vec3f& direction) {
			Transform transform = dirLightEntity->worldTransform;
			transform.rotation = quaternionFromVec3f(Vec3f(0.0f, 0.0f, 1.0f), direction);
			DirLightEntitySetWorldTransform(world, dirLightEntity, transform);
		}

		void DirLightEntitySetLocalTransform(World* world, DirLightEntity* dirLightEntity, const Transform& localTransform) {
			_CommonEntitySetLocalTransform((Entity*)dirLightEntity, localTransform);
			Vec3f direction = mat4Quaternion(dirLightEntity->worldTransform.rotation) * Vec3f(0.0f, 0.0f, 1.0f);
			dirLightEntity->spec.direction = direction;
			world->renderSystem.dirLightSetDirection(dirLightEntity->lightRID, direction);
		}

		void DirLightEntitySetWorldTransform(World* world, DirLightEntity* dirLightEntity, const Transform& worldTransform) {
			_CommonEntitySetWorldTransform((Entity*)dirLightEntity, worldTransform);
			Vec3f direction = mat4Quaternion(dirLightEntity->worldTransform.rotation) * Vec3f(0.0f, 0.0f, 1.0f);
			dirLightEntity->spec.direction = direction;
			world->renderSystem.dirLightSetDirection(dirLightEntity->lightRID, direction);
		}
	
	}
}