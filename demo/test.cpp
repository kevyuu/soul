#define VK_USE_PLATFORM_MACOS_MVK

#include "core/type.h"
#include "core/dev_util.h"
#include "core/array.h"
#include "core/math.h"
#include "job/system.h"

#include <cstring>
#include <algorithm>

#include "gpu/gpu.h"
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include <vector>

#include "utils.h"


#include "imgui_render_module.h"
#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_glfw.h>

#include <tiny_gltf.h>

using namespace Soul;

namespace Soul {

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

	struct MeshEntity : Entity {
		uint32 meshID;
		uint32 materialID;
	};

	struct Mesh {
		GPU::BufferID vertexBufferID;
		GPU::BufferID indexBufferID;
		uint16 indexCount;
	};

	struct DirectionalLight {
		Vec3f direction = Vec3f(0.0f, -1.0f, 0.0f);
		Vec3f color = { 1.0f, 1.0f, 1.0f };
		float illuminance = 100000; // in lx;
		float split[3] = { 0.1f, 0.3f, 0.6f };
		int32 shadowMapResolution = 2048;
		float bias = 0.001f;
	};

	enum class TexChannel : uint8 {
		RED,
		GREEN,
		BLUE,
		ALPHA,
		COUNT
	};

	struct SceneMaterial {
		char name[1024];

		PoolID albedoTexID;
		PoolID normalTexID;
		PoolID metallicTexID;
		PoolID roughnessTexID;
		PoolID aoTexID;
		PoolID emissiveTexID;

		Vec3f albedo;
		float metallic;
		float roughness;
		Vec3f emissive;

		bool useAlbedoTex;
		bool useNormalTex;
		bool useMetallicTex;
		bool useRoughnessTex;
		bool useAOTex;
		bool useEmissiveTex;

		TexChannel metallicTextureChannel;
		TexChannel roughnessTextureChannel;
		TexChannel aoTextureChannel;
	};

	struct SceneTexture {
		char name[1024];
		GPU::TextureID rid;
	};

	struct Scene {
		EntityID rootEntityID;

		PoolArray<GroupEntity> groupEntities;
		Array<MeshEntity> meshEntities;
		Array<Mesh> meshes;
		Array<SceneMaterial> materials;
		
		Array<SceneTexture> textures;
		DirectionalLight light;

		GPU::BufferID materialBuffer;
	};

	using TextureID = uint32;

};

Entity* EntityPtr(Scene* scene, EntityID entityID) {
	switch (entityID.type) {
	case EntityType_MESH:
		return scene->meshEntities.ptr(entityID.index);
	case EntityType_GROUP:
		return scene->groupEntities.ptr(entityID.index);
	default:
		SOUL_ASSERT(0, false, "Entity type is not valid, entity type = %d", entityID.type);
		return nullptr;
	}
}

EntityID EntityCreate(Scene* scene, EntityID parentID, EntityType entityType, const char* name, Transform localTransform) {
	EntityID entityID;
	entityID.type = entityType;
	Entity* entity = nullptr;
	switch (entityType) {
	case EntityType_MESH:
		entityID.index = scene->meshEntities.add(MeshEntity());
		entity = scene->meshEntities.ptr(entityID.index);
		break;
	case EntityType_GROUP:
		entityID.index = scene->groupEntities.add(GroupEntity());
		entity = scene->groupEntities.ptr(entityID.index);
		break;
	default:
		SOUL_ASSERT(0, false, "Entity type is not valid, entity type = %d", entityType);
	}

	GroupEntity* parent = (GroupEntity*)EntityPtr(scene, parentID);
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

void LoadScene(GPU::System* gpuSystem, Scene* scene, const char* path, bool positionToAABBCenter) {

	scene->groupEntities.reserve(3000);
	PoolID rootIndex = scene->groupEntities.add({});
	scene->rootEntityID.index = rootIndex;
	scene->rootEntityID.type = EntityType_GROUP;
	GroupEntity* rootEntity = scene->groupEntities.ptr(rootIndex);
	rootEntity->entityID = scene->rootEntityID;
	rootEntity->first = nullptr;
	strcpy(rootEntity->name, "Root");
	rootEntity->prev = nullptr;
	rootEntity->next = nullptr;
	rootEntity->localTransform = transformIdentity();
	rootEntity->worldTransform = transformIdentity();
	
	scene->meshEntities.reserve(10000);
	scene->meshEntities.add({});

	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);

	if (!warn.empty()) {
		SOUL_LOG_WARN("ImportGLTFAssets | %s", warn.c_str());
	}

	if (!err.empty()) {
		SOUL_LOG_ERROR("ImportGLTFAssets | %s", err.c_str());
		return;
	}

	if (!ret) {
		SOUL_LOG_ERROR("ImportGLTFAssets | failed");
		return;
	}

	//Load Textures
	Array<PoolID> textureIDs;
	textureIDs.resize(model.textures.size());
	for (int i = 0; i < model.textures.size(); i++) {
		const tinygltf::Texture& texture = model.textures[i];
		const tinygltf::Image& image = model.images[texture.source];

		GPU::TextureDesc texDesc;
		texDesc.width = image.width;
		texDesc.height = image.height;
		texDesc.depth = 1;
		texDesc.type = GPU::TextureType::D2;
		texDesc.format = GPU::TextureFormat::RGBA8;
		texDesc.mipLevels = 1;
		texDesc.usageFlags = GPU::TEXTURE_USAGE_SAMPLED_BIT;
		texDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;

		GPU::TextureID textureID = gpuSystem->textureCreate(texDesc, (byte*) &image.image[0], image.image.size());

		PoolID sceneTextureID = scene->textures.add({});
		SceneTexture& sceneTexture = scene->textures[sceneTextureID];
		strcpy(sceneTexture.name, texture.name.c_str());
		sceneTexture.rid = textureID;
		textureIDs[i] = sceneTextureID;
	}

	//Load Material
	SOUL_LOG_INFO("Materials count : %d", model.materials.size());
	Array<PoolID> materialIDs;
	materialIDs.resize(model.materials.size());
	for (int i = 0; i < model.materials.size(); i++) {
		SceneMaterial sceneMaterial = {};
		const tinygltf::Material& material = model.materials[i];
		
		if (material.values.count("baseColorFactor") != 0) {
			const tinygltf::ColorValue& colorValue = material.values.at("baseColorFactor").ColorFactor();
			sceneMaterial.albedo = {
				(float)colorValue[0],
				(float)colorValue[1],
				(float)colorValue[2]
			};
		}

		if (material.values.count("baseColorTexture") != 0) {
			int texID = material.values.at("baseColorTexture").TextureIndex();
			sceneMaterial.albedoTexID = textureIDs[texID];
			sceneMaterial.useAlbedoTex = true;
		}

		if (material.values.count("metallicFactor") != 0) {
			sceneMaterial.metallic = material.values.at("metallicFactor").Factor();
		}
		else {
			sceneMaterial.metallic = 0.0f;
		}

		if (material.values.count("roughnessFactor") != 0) {
			sceneMaterial.roughness = material.values.at("roughnessFactor").Factor();
		}
		else {
			sceneMaterial.roughness = 0.0f;
		}

		if (material.values.count("metallicRoughnessTexture") != 0) {
			int texID = material.values.at("metallicRoughnessTexture").TextureIndex();

			sceneMaterial.metallicTexID = textureIDs[texID];
			sceneMaterial.metallicTextureChannel = TexChannel::BLUE;
			sceneMaterial.useMetallicTex = true;

			sceneMaterial.roughnessTexID = textureIDs[texID];
			sceneMaterial.roughnessTextureChannel = TexChannel::GREEN;
			sceneMaterial.useRoughnessTex = true;
		}

		if (material.additionalValues.count("normalTexture")) {
			int texID = material.additionalValues.at("normalTexture").TextureIndex();

			sceneMaterial.normalTexID = textureIDs[texID];
			sceneMaterial.useNormalTex = true;

		}

		if (material.additionalValues.count("occlusionTexture")) {
			int texID = material.additionalValues.at("occlusionTexture").TextureIndex();
			sceneMaterial.aoTexID = textureIDs[texID];
			sceneMaterial.useAOTex = true;
			sceneMaterial.aoTextureChannel = TexChannel::RED;
		}

		if (material.additionalValues.count("emissiveTexture")) {
			int texID = material.additionalValues.at("emissiveTexture").TextureIndex();
			sceneMaterial.emissiveTexID = textureIDs[texID];
			sceneMaterial.useEmissiveTex = true;
		}

		if (material.additionalValues.count("emissiveFactor")) {
			const tinygltf::ColorValue& colorValue = material.additionalValues.at("emissiveFactor").ColorFactor();
			sceneMaterial.emissive = {
				(float)colorValue[0],
				(float)colorValue[1],
				(float)colorValue[2]
			};
		}
		else {
			sceneMaterial.emissive = Vec3f(0.0f, 0.0f, 0.0f);
		}

		SOUL_ASSERT(0, strlen(material.name.c_str()) <= 512, "Material name is too long | material.name = %s", material.name.c_str());
		strcpy(sceneMaterial.name, material.name.c_str());

		PoolID poolID = scene->materials.add(sceneMaterial);
		materialIDs[i] = poolID;
	}

	struct MaterialData {
		Vec3f albedo;
		float metallic;
		Vec3f emissive;
		float roughness;
		uint32 flags;
	};

	enum MaterialFlag {
		MaterialFlag_USE_ALBEDO_TEX = (1UL << 0),
		MaterialFlag_USE_NORMAL_TEX = (1UL << 1),
		MaterialFlag_USE_METALLIC_TEX = (1UL << 2),
		MaterialFlag_USE_ROUGHNESS_TEX = (1UL << 3),
		MaterialFlag_USE_AO_TEX = (1UL << 4),
		MaterialFlag_USE_EMISSIVE_TEX = (1UL << 5),

		MaterialFlag_METALLIC_CHANNEL_RED = (1UL << 8),
		MaterialFlag_METALLIC_CHANNEL_GREEN = (1UL << 9),
		MaterialFlag_METALLIC_CHANNEL_BLUE = (1UL << 10),
		MaterialFlag_METALLIC_CHANNEL_ALPHA = (1UL << 11),

		MaterialFlag_ROUGHNESS_CHANNEL_RED = (1UL << 12),
		MaterialFlag_ROUGHNESS_CHANNEL_GREEN = (1UL << 13),
		MaterialFlag_ROUGHNESS_CHANNEL_BLUE = (1UL << 14),
		MaterialFlag_ROUGHNESS_CHANNEL_ALPHA = (1UL << 15),

		MaterialFlag_AO_CHANNEL_RED = (1UL << 16),
		MaterialFlag_AO_CHANNEL_GREEN = (1UL << 17),
		MaterialFlag_AO_CHANNEL_BLUE = (1UL << 18),
		MaterialFlag_AO_CHANNEL_ALPHA = (1UL << 19)
	};

	GPU::BufferDesc bufferDesc;
	bufferDesc.typeSize = sizeof(MaterialData);
	bufferDesc.typeAlignment = alignof(MaterialData);
	bufferDesc.count = scene->materials.size();
	bufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
	bufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
	scene->materialBuffer = gpuSystem->bufferCreate(bufferDesc,
			[scene](int index, byte* data) {
		auto materialData = (MaterialData*) data;
		SceneMaterial& sceneMaterial = scene->materials[index];

		materialData->albedo = sceneMaterial.albedo;
		materialData->metallic = sceneMaterial.metallic;
		materialData->emissive = sceneMaterial.emissive;
		materialData->roughness = sceneMaterial.roughness;

		int flags = 0;
		if (sceneMaterial.useAlbedoTex) flags |= MaterialFlag_USE_ALBEDO_TEX;
		if (sceneMaterial.useNormalTex) flags |= MaterialFlag_USE_NORMAL_TEX;
		if (sceneMaterial.useMetallicTex) flags |= MaterialFlag_USE_METALLIC_TEX;
		if (sceneMaterial.useRoughnessTex) flags |= MaterialFlag_USE_ROUGHNESS_TEX;
		if (sceneMaterial.useAOTex) flags |= MaterialFlag_USE_AO_TEX;
		if (sceneMaterial.useEmissiveTex) flags |= MaterialFlag_USE_EMISSIVE_TEX;

		materialData->flags = flags;
	});

	Array<EntityID> entityParents;
	entityParents.reserve(model.nodes.size());
	Array<EntityID> meshEntityIDs;
	meshEntityIDs.reserve(model.meshes.size());

	for (int i = 0; i < model.nodes.size(); i++) {
		entityParents.add(scene->rootEntityID);
	}

	for (int i = 0; i < model.meshes.size(); i++) {
		meshEntityIDs.add({ 0 });
	}

	EntityID firstEntityID = { 0 };

	// LoadNode
	for (int i = 0; i < model.nodes.size(); i++) {
		
		const tinygltf::Node& gltfNode = model.nodes[i];

		EntityID entityID;

		Transform localNodeTransform;
		if (gltfNode.matrix.size() == 16) {
			Mat4 temp = mat4(gltfNode.matrix.data());
			localNodeTransform = transformMat4(mat4Transpose(temp));
			localNodeTransform.rotation = unit(localNodeTransform.rotation);
		}
		else {
			if (gltfNode.translation.size() == 3) {
				localNodeTransform.position = Vec3f(gltfNode.translation[0], gltfNode.translation[1], gltfNode.translation[2]);
			}
			else {
				localNodeTransform.position = Vec3f(0.0f, 0.0f, 0.0f);
			}
			if (gltfNode.scale.size() == 3) {
				localNodeTransform.scale = Vec3f(gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]);
			}
			else {
				localNodeTransform.scale = Vec3f(1.0f, 1.0f, 1.0f);
			}
			if (gltfNode.rotation.size() == 4) {
				localNodeTransform.rotation = Quaternion(gltfNode.rotation[0], gltfNode.rotation[1], gltfNode.rotation[2], gltfNode.rotation[3]);
			}
			else {
				localNodeTransform.rotation = quaternionIdentity();
			}
		}

		if (gltfNode.mesh > -1) {
			entityID = EntityCreate(scene, entityParents[i], EntityType_MESH, gltfNode.name.c_str(), localNodeTransform);
			meshEntityIDs[gltfNode.mesh] = entityID;
			SOUL_ASSERT(0, gltfNode.children.size() == 0, "Node containing mesh must not have children node. | Mesh index : %d", i);
		}
		else {
			entityID = EntityCreate(scene, entityParents[i], EntityType_GROUP, gltfNode.name.c_str(), localNodeTransform);
		}

		if (i == 0) firstEntityID = entityID;

		for (int j = 0; j < gltfNode.children.size(); j++) {
			entityParents[gltfNode.children[j]] = entityID;
		}

	}

	entityParents.cleanup();

	// Load Mesh
	for (int i = 0; i < model.meshes.size(); i++) {

		struct Vertex {
			Vec3f pos;
			Vec3f normal;
			Vec2f texUV;
			Vec3f binormal;
			Vec3f tangent;
		};

		Soul::Array<Vertex> vertexes;
		vertexes.reserve(100000);
		using Index = uint32;
		Soul::Array<Index> indexes;
		indexes.reserve(100000);

		MeshEntity *meshEntity = (MeshEntity*)EntityPtr(scene, meshEntityIDs[i]);

		const tinygltf::Mesh& mesh = model.meshes[i];
		SOUL_ASSERT(0, mesh.primitives.size() == 1, "Mesh with multiple number of primitive is not supported yet | mesh name = %s, number of primitive = %d", mesh.name.c_str(), mesh.primitives.size());

		const tinygltf::Primitive& primitive = mesh.primitives[0];

		const tinygltf::Accessor& positionAccessor = model.accessors[primitive.attributes.at("POSITION")];
		const tinygltf::Accessor& normalAccessor = model.accessors[primitive.attributes.at("NORMAL")];
		const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];

		SOUL_ASSERT(0, positionAccessor.count == normalAccessor.count, "");

		SOUL_ASSERT(0, positionAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Component type %d for position is not supported yet. | mesh name = %s.", positionAccessor.componentType, mesh.name.c_str());
		SOUL_ASSERT(0, positionAccessor.type == TINYGLTF_TYPE_VEC3, "Type %d for position is not supported yet. | mesh name = %s.", positionAccessor.type, mesh.name.c_str());
		const tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccessor.bufferView];
		int positionOffset = positionAccessor.byteOffset + positionBufferView.byteOffset;
		int positionByteStride = positionAccessor.ByteStride(positionBufferView);
		unsigned char* positionBuffer = &model.buffers[positionBufferView.buffer].data[positionOffset];

		SOUL_ASSERT(0, normalAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Component type %d for normal is not supported yet. | mesh name = %s.", normalAccessor.componentType, mesh.name.c_str());
		SOUL_ASSERT(0, normalAccessor.type == TINYGLTF_TYPE_VEC3, "Type %d for normal is not supported yet. | mesh name = %s.", normalAccessor.type, mesh.name.c_str());
		const tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccessor.bufferView];
		int normalOffset = normalAccessor.byteOffset + normalBufferView.byteOffset;
		int normalByteStride = normalAccessor.ByteStride(normalBufferView);
		unsigned char* normalBuffer = &model.buffers[normalBufferView.buffer].data[normalOffset];

		AABB meshAABB;
		Soul::Vec3f position0 = *(Soul::Vec3f*)&positionBuffer[0];
		meshAABB.min = meshEntity->worldTransform * position0;
		meshAABB.max = meshAABB.min;

		for (int i = 1; i < positionAccessor.count; i++) {

			Soul::Vec3f position = *(Soul::Vec3f*)&positionBuffer[positionByteStride * i];

			Soul::Vec3f worldPosition = meshEntity->worldTransform * position;

			meshAABB.min = componentMin(meshAABB.min, worldPosition);
			meshAABB.max = componentMax(meshAABB.max, worldPosition);
		}

		Mat4 vertexPositionTransform = mat4Identity();
		if (positionToAABBCenter) {
			vertexPositionTransform = mat4Transform(meshEntity->worldTransform);
			Vec3f meshAABBCenter = (meshAABB.min + meshAABB.max) / 2.0f;
			meshEntity->worldTransform.position = meshAABBCenter;
			Mat4 localMat4 = mat4Inverse(mat4Transform(meshEntity->parent->worldTransform)) * mat4Transform(meshEntity->worldTransform);
			meshEntity->localTransform = transformMat4(localMat4);
			vertexPositionTransform = mat4Inverse(mat4Transform(meshEntity->worldTransform)) * vertexPositionTransform;
			
		}

		Array<Vec2f> texCoord0s;
		texCoord0s.reserve(positionAccessor.count);
		if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
			const tinygltf::Accessor& texCoord0Accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
			SOUL_ASSERT(0, texCoord0Accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Component type %d for texCoord0 is not supported yet. | mesh name = %s.", texCoord0Accessor.componentType, mesh.name.c_str());
			SOUL_ASSERT(0, texCoord0Accessor.type == TINYGLTF_TYPE_VEC2, "Type %d for texCoord0 is not supported yet. | mesh name = %s.", texCoord0Accessor.type, mesh.name.c_str());
			const tinygltf::BufferView& texCoord0BufferView = model.bufferViews[texCoord0Accessor.bufferView];
			int texCoord0Offset = texCoord0Accessor.byteOffset + texCoord0BufferView.byteOffset;
			int texCoord0ByteStride = texCoord0Accessor.ByteStride(texCoord0BufferView);
			unsigned char* texCoord0Buffer = &model.buffers[texCoord0BufferView.buffer].data[texCoord0Offset];
			for (int i = 0; i < positionAccessor.count; i++) {
				texCoord0s.add(*(Soul::Vec2f*)&texCoord0Buffer[texCoord0ByteStride * i]);
			}
		}
		else {
			texCoord0s.add(Vec2f(0.0f, 0.0f));
		}
		
		Array<Vec4f> tangents;
		tangents.reserve(positionAccessor.count);
		if (primitive.attributes.find("TANGENT") != primitive.attributes.end()) {
			const tinygltf::Accessor& tangentAccessor = model.accessors[primitive.attributes.at("TANGENT")];
			SOUL_ASSERT(0, tangentAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Component type %d for tangent is not supported yet. | mesh name = %s.", tangentAccessor.componentType, mesh.name.c_str());
			SOUL_ASSERT(0, tangentAccessor.type == TINYGLTF_TYPE_VEC4, "Type %d for tangent is not supported yet. | mesh name = %s.", tangentAccessor.type, mesh.name.c_str());
			const tinygltf::BufferView& tangentBufferView = model.bufferViews[tangentAccessor.bufferView];
			int tangentOffset = tangentAccessor.byteOffset + tangentBufferView.byteOffset;
			int tangentByteStride = tangentAccessor.ByteStride(tangentBufferView);
			unsigned char* tangentBuffer = &model.buffers[tangentBufferView.buffer].data[tangentOffset];
			for (int i = 0; i < positionAccessor.count; i++) {
				tangents.add(*(Soul::Vec4f*)&tangentBuffer[tangentByteStride * i]);
			}
		}
		else {
			for (int i = 0; i < positionAccessor.count; i++) {
				tangents.add(Vec4f(0.0f, 1.0f, 0.0f, 1.0f));
			}
		}
		
		for (int i = 0; i < positionAccessor.count; i++) {

			Soul::Vec3f position = *(Soul::Vec3f*)&positionBuffer[positionByteStride * i];
			Soul::Vec3f normal = *(Soul::Vec3f*)&normalBuffer[normalByteStride * i];
			Soul::Vec4f tangent = tangents[i];
			Soul::Vec2f texCoord0 = texCoord0s[i];

			vertexes.add({
				vertexPositionTransform * position,
				normal,
				texCoord0,
				Soul::cross(normal, tangent.xyz()),
				tangent.xyz()
			});

		}

		SOUL_ASSERT(0, indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, "Component type %d for index is not supported. | mesh name = %s.", indexAccessor.componentType, mesh.name.c_str());
		SOUL_ASSERT(0, indexAccessor.type == TINYGLTF_TYPE_SCALAR, "Type %d for index is not supported. | mesh name = %s.", indexAccessor.type, mesh.name.c_str());
		const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
		int indexOffset = indexAccessor.byteOffset + indexBufferView.byteOffset;
		int indexByteStride = indexAccessor.ByteStride(indexBufferView);
		unsigned char* indexBuffer = &model.buffers[indexBufferView.buffer].data[indexOffset];

		for (int i = 0; i < indexAccessor.count; ++i) {
			uint32 index = *(uint32*)&indexBuffer[indexByteStride * i];
			indexes.add(index);
		}

		Array<SceneMaterial>& materials = scene->materials;
		Mesh sceneMesh = {};

		GPU::BufferDesc vertexBufferDesc;
		vertexBufferDesc.typeSize = sizeof(Vertex);
		vertexBufferDesc.typeAlignment = alignof(Vertex);
		vertexBufferDesc.count = vertexes.size();
		vertexBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		vertexBufferDesc.usageFlags = GPU::BUFFER_USAGE_VERTEX_BIT;
		sceneMesh.vertexBufferID = gpuSystem->bufferCreate(vertexBufferDesc,
				[&vertexes](int index, byte* data) {
			auto vertex = (Vertex*) data;
			(*vertex) = vertexes[index];
		});

		GPU::BufferDesc indexBufferDesc;
		indexBufferDesc.typeSize = sizeof(Index);
		indexBufferDesc.typeAlignment = alignof(Index);
		indexBufferDesc.count = indexes.size();
		indexBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		indexBufferDesc.usageFlags = GPU::BUFFER_USAGE_INDEX_BIT;
		sceneMesh.indexBufferID = gpuSystem->bufferCreate(indexBufferDesc,
				[&indexes](int i, byte* data) {
			auto index = (Index*) data;
			(*index) = indexes[i];
		});
		sceneMesh.indexCount = indexes.size();

		meshEntity->meshID = scene->meshes.add(sceneMesh);
		meshEntity->materialID = materialIDs[primitive.material];

		texCoord0s.cleanup();
		tangents.cleanup();
		vertexes.cleanup();
		indexes.cleanup();
	}
	textureIDs.cleanup();
	materialIDs.cleanup();
	entityParents.cleanup();
	meshEntityIDs.cleanup();
}

void glfwPrintErrorCallback(int code, const char* message) {
	SOUL_LOG_INFO("GLFW Error. Error code : %d. Message = %s", code, message);
}

int main() {
	
	Job::System::Get().init({
		0,
		4096
	});

	
	glfwSetErrorCallback(glfwPrintErrorCallback);
	SOUL_ASSERT(0, glfwInit(), "GLFW Init Failed !");
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	SOUL_LOG_INFO("GLFW initialization sucessful");

	SOUL_ASSERT(0, glfwVulkanSupported(), "Vulkan not supported by glfw");

	GLFWwindow* window = glfwCreateWindow(2880, 1800, "Vulkan", nullptr, nullptr);
	SOUL_LOG_INFO("GLFW window creation sucessful");

	Soul::GPU::System gpuSystem;
	Soul::GPU::System::Config config = {};
	config.windowHandle = window;
	config.swapchainWidth = 3360;
	config.swapchainHeight = 2010;
	config.maxFrameInFlight = 3;
	config.threadCount = Job::System::Get().getThreadCount();

	gpuSystem.init(config);

	Scene scene;
	LoadScene(&gpuSystem, &scene,
			"sponza/scene.gltf",
			true);

	const char* renderAlbedoVertSrc = LoadFile("render_albedo.vert.glsl");
	const char* renderAlbedoFragSrc = LoadFile("render_albedo.frag.glsl");
	GPU::ShaderDesc vertShaderDesc;
	vertShaderDesc.name = "Render albedo vertex";
	vertShaderDesc.source = renderAlbedoVertSrc;
	vertShaderDesc.sourceSize = strlen(renderAlbedoVertSrc);
	GPU::ShaderID vertShaderID = gpuSystem.shaderCreate(vertShaderDesc, GPU::ShaderStage::VERTEX);

	GPU::ShaderDesc fragShaderDesc;
	fragShaderDesc.name = "Render albedo frag";
	fragShaderDesc.source = renderAlbedoFragSrc;
	fragShaderDesc.sourceSize = strlen(renderAlbedoFragSrc);
	GPU::ShaderID fragShaderID = gpuSystem.shaderCreate(fragShaderDesc, GPU::ShaderStage::FRAGMENT);

	GPU::TextureDesc depthTexDesc = {};
	depthTexDesc.width = gpuSystem.getSwapchainExtent().x;
	depthTexDesc.height = gpuSystem.getSwapchainExtent().y;
	depthTexDesc.depth = 1;
	depthTexDesc.mipLevels = 1;
	depthTexDesc.format = GPU::TextureFormat::DEPTH24_STENCIL8UI;
	depthTexDesc.type = GPU::TextureType::D2;
	depthTexDesc.usageFlags = GPU::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depthTexDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
	GPU::TextureID depthTexID = gpuSystem.textureCreate(depthTexDesc);

	GPU::SamplerDesc samplerDesc = {};
	samplerDesc.minFilter = GPU::TextureFilter::LINEAR;
	samplerDesc.magFilter = GPU::TextureFilter::LINEAR;
	samplerDesc.mipmapFilter = GPU::TextureFilter::LINEAR;
	samplerDesc.wrapU = GPU::TextureWrap::CLAMP_TO_EDGE;
	samplerDesc.wrapV = GPU::TextureWrap::CLAMP_TO_EDGE;
	samplerDesc.wrapW = GPU::TextureWrap::CLAMP_TO_EDGE;
	samplerDesc.anisotropyEnable = false;
	samplerDesc.maxAnisotropy = 0.0f;
	GPU::SamplerID samplerID = gpuSystem.samplerRequest(samplerDesc);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGuiRenderModule imguiRenderModule;
	imguiRenderModule.init(&gpuSystem, gpuSystem.getSwapchainExtent());

	struct Camera {
		Vec3f up;
		Vec3f direction;
		Vec3f position;

		Mat4 projection;
		Mat4 view;

		uint16 viewportWidth;
		uint16 viewportHeight;

		float aperture = 1.0f;
		float shutterSpeed = 1.0f;
		float sensitivity = 1.0f;
		float exposure = 1.0f;

		bool exposureFromSetting = false;

		union {
			struct {
				float fov;
				float aspectRatio;
				float zNear;
				float zFar;
			} perspective;

			struct {
				float left;
				float right;
				float top;
				float bottom;
				float zNear;
				float zFar;
			} ortho;
		};

		void updateExposure() {
			if (exposureFromSetting) {
				float ev100 = log2((aperture * aperture) / shutterSpeed * 100.0 / sensitivity);
				exposure = 1.0f / (pow(2.0f, ev100) * 1.2f);
			}
		}

	};

	Camera camera;
	camera.position = Soul::Vec3f(1.0f, 1.0f, 0.0f);
	camera.direction = Soul::Vec3f(0.0f, 0.0f, -1.0f);
	camera.up = Soul::Vec3f(0.0f, 1.0f, 0.0f);
	camera.perspective.fov = Soul::PI / 4;
	camera.perspective.aspectRatio = config.swapchainWidth / config.swapchainHeight;
	camera.viewportWidth = config.swapchainWidth;
	camera.viewportHeight = config.swapchainHeight;
	camera.perspective.zNear = 0.1f;
	camera.perspective.zFar = 3000.0f;
	camera.projection = Soul::mat4Perspective(camera.perspective.fov,
		camera.perspective.aspectRatio,
		camera.perspective.zNear,
		camera.perspective.zFar);

	struct CameraUBO {
		Mat4 projection;
		Mat4 view;
		Mat4 projectionView;
		Mat4 invProjectionView;

		Vec3f position;
		float exposure;
	};
	CameraUBO cameraDataUBO;


	GPU::BufferDesc modelBufferDesc;
	modelBufferDesc.typeSize = sizeof(Mat4);
	modelBufferDesc.typeAlignment = alignof(Mat4);
	modelBufferDesc.count = scene.meshEntities.size();
	modelBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
	modelBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
	GPU::BufferID modelBuffer = gpuSystem.bufferCreate(modelBufferDesc,
		[&scene](int i, byte* data) {
		auto model = (Mat4*) data;
		*model = mat4Transpose(mat4Transform(scene.meshEntities[i].worldTransform));
	});

	while(!glfwWindowShouldClose(window)) {
		SOUL_PROFILE_FRAME();

		Job::System::Get().beginFrame();

		glfwPollEvents();

		Vec2ui32 extent = gpuSystem.getSwapchainExtent();

		SOUL_LOG_INFO("Extent x = %d, y = %d.", extent.x, extent.y);

		GPU::RenderGraph renderGraph;

		// Start the Dear ImGui frame
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Render();

		cameraDataUBO.projection = mat4Transpose(camera.projection);
		Mat4 viewMat = mat4View(camera.position, camera.position +
												 camera.direction, camera.up);
		cameraDataUBO.view = mat4Transpose(viewMat);
		Mat4 projectionView = camera.projection * viewMat;
		cameraDataUBO.projectionView = mat4Transpose(projectionView);
		Mat4 invProjectionView = mat4Inverse(projectionView);
		cameraDataUBO.invProjectionView = mat4Transpose(invProjectionView);
		cameraDataUBO.position = camera.position;
		cameraDataUBO.exposure = camera.exposure;

		GPU::BufferDesc cameraBufferDesc;
		cameraBufferDesc.typeSize = sizeof(CameraUBO);
		cameraBufferDesc.typeAlignment = alignof(CameraUBO);
		cameraBufferDesc.count = 1;
		cameraBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		cameraBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		GPU::BufferID cameraBuffer = gpuSystem.bufferCreate(cameraBufferDesc,
		[&cameraDataUBO](int i, byte* data) {
			auto cameraData = (CameraUBO*) data;
			*cameraData = cameraDataUBO;
		});

		gpuSystem.bufferDestroy(cameraBuffer);

		Array<GPU::TextureNodeID> sceneTextureNodeIDs;
		sceneTextureNodeIDs.reserve(scene.textures.size());
		for (const SceneTexture& sceneTexture : scene.textures) {
			sceneTextureNodeIDs.add(renderGraph.importTexture("Scene Texture", sceneTexture.rid));
		}

		GPU::BufferNodeID materialNodeID = renderGraph.importBuffer("Material buffer", scene.materialBuffer);
		GPU::BufferNodeID modelNodeID = renderGraph.importBuffer("Model buffer", modelBuffer);
		GPU::BufferNodeID cameraNodeID = renderGraph.importBuffer("Camera buffer", cameraBuffer);

		Array<GPU::BufferNodeID> vertexBufferNodeIDs;
		vertexBufferNodeIDs.reserve(scene.meshes.size());
		for (const Mesh& mesh : scene.meshes) {
			vertexBufferNodeIDs.add(renderGraph.importBuffer("Vertex buffer", mesh.vertexBufferID));
		}

		Array<GPU::BufferNodeID> indexBufferNodeIDs;
		indexBufferNodeIDs.reserve(scene.meshes.size());
		for (const Mesh& mesh : scene.meshes) {
			indexBufferNodeIDs.add(renderGraph.importBuffer("Index buffer", mesh.indexBufferID));
		}

		GPU::TextureNodeID renderTarget = renderGraph.importTexture("Render target", gpuSystem.getSwapchainTexture());
		GPU::TextureNodeID depthTarget = renderGraph.importTexture("Depth target", depthTexID);

		struct RenderAlbedoData {
			Array<GPU::BufferNodeID> vertexBuffers;
			Array<GPU::BufferNodeID> indexBuffers;
			Array<GPU::TextureNodeID> sceneTextures;
			GPU::BufferNodeID camera;
			GPU::BufferNodeID material;
			GPU::BufferNodeID model;
			GPU::TextureNodeID renderTarget;
			GPU::TextureNodeID depthAttachment;

			~RenderAlbedoData() {
				sceneTextures.cleanup();
				indexBuffers.cleanup();
				vertexBuffers.cleanup();
			}
		};

		renderGraph.addGraphicPass<RenderAlbedoData>("Render albedo",
		[&scene,
		   &sceneTextureNodeIDs, &vertexBufferNodeIDs, &indexBufferNodeIDs,
		   materialNodeID, modelNodeID, cameraNodeID,
		   vertShaderID, fragShaderID,
		   renderTarget, depthTarget,
		   extent]
		(GPU::GraphicNodeBuilder* builder, RenderAlbedoData* data) {

			for (GPU::TextureNodeID nodeID : sceneTextureNodeIDs) {
				data->sceneTextures.add(builder->addInShaderTexture(nodeID, 2, 0));
			}

			for (GPU::BufferNodeID nodeID : vertexBufferNodeIDs) {
				data->vertexBuffers.add(builder->addVertexBuffer(nodeID));
			}

			for (GPU::BufferNodeID nodeID : indexBufferNodeIDs) {
				data->indexBuffers.add(builder->addIndexBuffer(nodeID));
			}

			data->camera = builder->addInShaderBuffer(cameraNodeID, 0, 0);
			data->material = builder->addInShaderBuffer(materialNodeID, 1, 0);
			data->model = builder->addInShaderBuffer(modelNodeID, 3, 0);

			GPU::ColorAttachmentDesc colorAttchDesc;
			colorAttchDesc.blendEnable = true;
			colorAttchDesc.srcColorBlendFactor = GPU::BlendFactor::SRC_ALPHA;
			colorAttchDesc.dstColorBlendFactor = GPU::BlendFactor::ONE_MINUS_SRC_ALPHA;
			colorAttchDesc.colorBlendOp = GPU::BlendOp::ADD;
			colorAttchDesc.srcAlphaBlendFactor = GPU::BlendFactor::ONE;
			colorAttchDesc.dstAlphaBlendFactor = GPU::BlendFactor::ZERO;
			colorAttchDesc.alphaBlendOp = GPU::BlendOp::ADD;
			colorAttchDesc.clear = true;
			colorAttchDesc.clearValue.color = {1.0f, 0.0f, 0.0f, 1.0f};
			data->renderTarget = builder->addColorAttachment(renderTarget, colorAttchDesc);

			GPU::DepthStencilAttachmentDesc depthAttchDesc;
			depthAttchDesc.clear = true;
			depthAttchDesc.clearValue.depthStencil = {1.0f, 0.0f};
			depthAttchDesc.depthWriteEnable = true;
			depthAttchDesc.depthTestEnable = true;
			depthAttchDesc.depthCompareOp = GPU::CompareOp::LESS;
			data->depthAttachment = builder->setDepthStencilAttachment(depthTarget, depthAttchDesc);

			GPU::GraphicPipelineConfig pipelineConfig;
			pipelineConfig.viewport = {0, 0, uint16(extent.x), uint16(extent.y)};
			pipelineConfig.scissor = {false, 0, 0, uint16(extent.x), uint16(extent.y)};
			pipelineConfig.framebuffer = {uint16(extent.x), uint16(extent.y)};
			pipelineConfig.vertexShaderID = vertShaderID;
			pipelineConfig.fragmentShaderID = fragShaderID;
			pipelineConfig.raster.cullMode = GPU::CullMode::NONE;

			builder->setPipelineConfig(pipelineConfig);

		},
		[&scene, samplerID]
		(GPU::RenderGraphRegistry* registry, const RenderAlbedoData& data, GPU::CommandBucket* commandBucket){

			commandBucket->reserve(scene.meshEntities.size());

			GPU::Descriptor cameraDescriptor;
			cameraDescriptor.type = GPU::DescriptorType::UNIFORM_BUFFER;
			cameraDescriptor.uniformInfo.bufferID = registry->getBuffer(data.camera);
			cameraDescriptor.uniformInfo.unitIndex = 0;

			GPU::ShaderArgSetDesc set0Desc;
			set0Desc.bindingCount = 1;
			set0Desc.bindingDescriptions = &cameraDescriptor;
			GPU::ShaderArgSetID set0 = registry->getShaderArgSet(0, set0Desc);

			int meshOffset = 0;
			for (const MeshEntity& meshEntity : scene.meshEntities) {

				const SceneMaterial& material = scene.materials[meshEntity.materialID];
				GPU::Descriptor materialDescriptors[2] = {};
				materialDescriptors[0].type = GPU::DescriptorType::SAMPLED_IMAGE;
				if (material.useAlbedoTex) {
					materialDescriptors[0].sampledImageInfo.textureID = registry->getTexture(data.sceneTextures[material.albedoTexID]);
				} else {
					materialDescriptors[0].sampledImageInfo.textureID = registry->getTexture(data.sceneTextures[0]);
				}
				materialDescriptors[0].sampledImageInfo.samplerID = samplerID;
				materialDescriptors[1].type = GPU::DescriptorType::UNIFORM_BUFFER;
				materialDescriptors[1].uniformInfo.bufferID = registry->getBuffer(data.material);
				materialDescriptors[1].uniformInfo.unitIndex = meshEntity.materialID;

				GPU::ShaderArgSetDesc set1Desc;
				set1Desc.bindingCount = 1;
				set1Desc.bindingDescriptions = &materialDescriptors[1];
				GPU::ShaderArgSetID set1 = registry->getShaderArgSet(1, set1Desc);

				GPU::ShaderArgSetDesc set2Desc;
				set2Desc.bindingCount = 1;
				set2Desc.bindingDescriptions = materialDescriptors;
				GPU::ShaderArgSetID set2 = registry->getShaderArgSet(2, set2Desc);

				GPU::Descriptor modelDescriptor;
				modelDescriptor.type = GPU::DescriptorType::UNIFORM_BUFFER;
				modelDescriptor.uniformInfo.bufferID = registry->getBuffer(data.model);
				modelDescriptor.uniformInfo.unitIndex = meshOffset;
				GPU::ShaderArgSetDesc set3Desc;
				set3Desc.bindingCount = 1;
				set3Desc.bindingDescriptions = &modelDescriptor;
				GPU::ShaderArgSetID set3 = registry->getShaderArgSet(3, set3Desc);

				const Mesh& mesh = scene.meshes[meshEntity.meshID];
				using DrawCommand = GPU::Command::DrawIndex;
				DrawCommand* command = commandBucket->put<DrawCommand>(meshOffset, meshOffset);
				command->vertexBufferID = registry->getBuffer(data.vertexBuffers[meshEntity.meshID]);
				command->indexBufferID = registry->getBuffer(data.indexBuffers[meshEntity.meshID]);
				command->indexCount = mesh.indexCount;
				command->shaderArgSets[0] = set0;
				command->shaderArgSets[1] = set1;
				command->shaderArgSets[2] = set2;
				command->shaderArgSets[3] = set3;

				++meshOffset;
			}

		});

		gpuSystem.renderGraphExecute(renderGraph);
		gpuSystem.frameFlush();
		renderGraph.cleanup();

		vertexBufferNodeIDs.cleanup();
		indexBufferNodeIDs.cleanup();
		sceneTextureNodeIDs.cleanup();

			static float translationSpeed = 0.025f;

			float cameraSpeedInc = 0.1f;
			translationSpeed += (cameraSpeedInc * translationSpeed * io.MouseWheel);


			if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
				translationSpeed *= 0.9f;
			}
			if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
				translationSpeed *= 1.1f;
			}

			if (ImGui::IsMouseDragging(2)) {

				Soul::Vec3f cameraRight = cross(camera.up, camera.direction) * -1.0f;

				{
					Soul::Mat4 rotate = mat4Rotate(cameraRight, -2.0f * io.MouseDelta.y / camera.viewportHeight * Soul::PI);
					camera.direction = rotate * camera.direction;
					camera.up = rotate * camera.up;
				}

				{
					Soul::Mat4 rotate = mat4Rotate(Soul::Vec3f(0.0f, 1.0f, 0.0f), -2.0f * io.MouseDelta.x / camera.viewportWidth *Soul::PI);

					if (camera.direction != Soul::Vec3f(0.0f, 1.0f, 0.0f))
						camera.direction = rotate * camera.direction;
					if (camera.up != Soul::Vec3f(0.0f, 1.0f, 0.0f))
						camera.up = rotate * camera.up;
				}

			}

			Soul::Vec3f right = Soul::unit(cross(camera.direction, camera.up));
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
				camera.position += Soul::unit(camera.direction) * translationSpeed;
			}
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
				camera.position -= Soul::unit(camera.direction) * translationSpeed;
			}
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
				camera.position -= Soul::unit(right) * translationSpeed;
			}
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
				camera.position += Soul::unit(right) * translationSpeed;
			}

		camera.view = mat4View(camera.position, camera.position + camera.direction, camera.up);
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
