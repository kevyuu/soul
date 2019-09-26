#define VK_USE_PLATFORM_MACOS_MVK

#include "core/type.h"
#include "core/debug.h"
#include "core/array.h"
#include "core/math.h"
#include "job/system.h"

#include "gpu/render_graph.h"
#include "gpu/system.h"

#include <cstdio>
#include <cstring>
#include <algorithm>

#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

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
		PoolArray<MeshEntity> meshEntities;
		PoolArray<Mesh> meshes;
		PoolArray<SceneMaterial> materials;
		
		PoolArray<SceneTexture> textures;
		DirectionalLight light;
	};

	using TextureID = uint32;

};

using namespace Soul;

Entity* EntityPtr(Scene* scene, EntityID entityID) {
	switch (entityID.type) {
	case EntityType_MESH:
		return scene->meshEntities.ptr(entityID.index);
	case EntityType_GROUP:
		return scene->groupEntities.ptr(entityID.index);
	default:
		SOUL_ASSERT(0, false, "Entity type is not valid, entity type = %d", entityID.type);
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

		GPU::TextureID textureID = gpuSystem->samplerTextureCreate({
			.format = GPU::TextureFormat::RGBA8,
			.width = (uint16) image.width,
			.height = (uint16)image.height,
			.mipLevels = 8,
			.dataSize = (uint32)image.image.size(),
			.data = (unsigned char*) &image.image[0]
		});

		PoolID sceneTextureID = scene->textures.add({});
		SceneTexture& sceneTexture = scene->textures[sceneTextureID];
		strcpy(sceneTexture.name, texture.name.c_str());
		sceneTexture.rid = textureID;
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

	gpuSystem->uniformBufferArrayCreate<MaterialData>(
		scene->materials.size(), 
		[scene](int index, MaterialData* data) {
			data->albedo = scene->materials[index].albedo;
			data->metallic = scene->materials[index].metallic;
			data->emissive = scene->materials[index].emissive;
			data->roughness = scene->materials[index].roughness;
		}
	);

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
			tangents.add(Vec4f(0.0f, 1.0f, 0.0f, 1.0f));
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

		PoolArray<SceneMaterial>& materials = scene->materials;

		Mesh sceneMesh = {
			.vertexBufferID = gpuSystem->vertexBufferCreate({
				.size = vertexes.size() * (uint32) sizeof(Vertex),
				.data = vertexes.data()
			}),
			.indexBufferID = gpuSystem->indexBufferCreate({
				.size = indexes.size() * (uint32) sizeof(Index),
				.data = indexes.data()
			})
		};

		meshEntity->meshID = scene->meshes.add(sceneMesh);
		meshEntity->materialID = materialIDs[primitive.material];

		texCoord0s.cleanup();
		tangents.cleanup();
		vertexes.cleanup();
		indexes.cleanup();
	}

	meshEntityIDs.cleanup();
	textureIDs.cleanup();
	materialIDs.cleanup();
	
}

int main() {
	
	Job::System::Get().init({
		0,
		4096
	});
	
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	SOUL_LOG_INFO("GLFW initialization sucessful");

	SOUL_ASSERT(0, glfwVulkanSupported(), "Vulkan is not supported by glfw!");

	GLFWwindow* window = glfwCreateWindow(1800, 720, "Vulkan", nullptr, nullptr);
	SOUL_LOG_INFO("GLFW window creation sucessful");

	Soul::GPU::System gpuSystem;
	Soul::GPU::System::Config config {
		.windowHandle = window,
		.swapchainWidth = 1800,
		.swapchainHeight = 720,
		.maxFrameInFlight = 3,
		.threadCount = Job::System::Get().getThreadCount()
	};
	gpuSystem.init(config);

	Scene scene;

	LoadScene(&gpuSystem, &scene, "/Users/utamak/Dev/personal/soul/soul/assets/sponza/scene.gltf", false);
	
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		/*GPU::RenderGraph renderGraph;

		struct UploadModelUniformData {
			GPU::BufferNodeID modelBuffer;
		};

		UploadModelUniformData uploadData = renderGraph.addTransferPass<UploadModelUniformData>("Upload uniform data", 
			[&scene](GPU::RenderGraph* renderGraph, UploadModelUniformData* data) {
				data->modelBuffer = renderGraph->createBuffer<Transform>({
					.count = scene.entities.size()
				}, GPU::BufferWriteUsage::TRANSFER_DST);
			},
			[&scene](GPU::System* gpuSystem, 
				GPU::CommandBucket* commandBucket,
				const UploadModelUniformData& data) {
				GPU::BufferID bufferID = gpuSystem->getBuffer(data.modelBuffer);
				commandBucket->reserve(scene.entities.size());
				for (int i = 0; i < scene.entities.size(); i++) {
					using UpdateCommand = GPU::Command::UpdateTypeBuffer<Transform>;
					UpdateCommand* command = commandBucket->put<UpdateCommand>(i, 0);
					command->bufferID = bufferID;
					command->data = &scene.entities[i].transform;
					command->index = i;
				}
			}
		).data;

		struct RenderSceneData {
			GPU::BufferNodeID modelBuffer;
			GPU::TextureNodeID depthTarget;
			GPU::TextureNodeID renderTarget;
		};

		renderGraph.addGraphicPass<RenderSceneData>("Render scene data",
			[uploadData](GPU::RenderGraph* renderGraph, GPU::GraphicPipelineDesc* desc, RenderSceneData* data) {
				data->modelBuffer = renderGraph->readBuffer(uploadData.modelBuffer, GPU::BufferReadUsage::UNIFORM);
				data->renderTarget = GPU::RenderGraph::SWAPCHAIN_TEXTURE;
				data->depthTarget = renderGraph->createTexture("Final Depth Target", {
					.width = 1800,
					.height = 720,
					.format = GPU::TextureFormat::DEPTH32F,
					.clear = true,
					.clearValue = {
						.color = {},
						.depthStencil = {0.0f, 0.0f}
					}
				}, GPU::TextureWriteUsage::DEPTH_ATTACHMENT);

				desc->shaderDesc = {
					.vertexPath = "/Users/utamak/Dev/personal/soul/soul/shaders/gbuffer_gen.vert.glsl",
					.geometryPath = nullptr,
					.fragmentPath = "/Users/utamak/Dev/personal/soul/soul/shaders/render_scene_basic.frag.glsl"
				};
				desc->inputLayoutDesc = {
					.topology = GPU::Topology::TRIANGLE_LIST
				};
				desc->rasterDesc = {
					.lineWidth = 1.0f
				};
				desc->depthStencilDesc.attachment = data->depthTarget;
				desc->colorDesc.attachments[0] = data->renderTarget;
			},
			[&scene](GPU::System* gpuSystem,
				GPU::CommandBucket* commandBucket,
				const RenderSceneData& data){
				
			}
		);*/
		
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
