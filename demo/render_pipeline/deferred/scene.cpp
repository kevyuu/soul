#include "data.h"

#include "core/dev_util.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"

#include <tiny_gltf.h>

#include <imgui/imgui.h>
#include <GLFW/glfw3.h>

using namespace Soul;

void DeferredPipeline::Scene::importFromGLTF(const char* path) {
	SOUL_PROFILE_ZONE();
	Memory::ScopeAllocator<> scopeAllocator("Load scene allocator");

	groupEntities.reserve(3000);
	PoolID rootIndex = groupEntities.add({});
	rootEntityID.index = rootIndex;
	rootEntityID.type = EntityType_GROUP;
	GroupEntity* rootEntity = groupEntities.ptr(rootIndex);
	rootEntity->entityID = rootEntityID;
	rootEntity->first = nullptr;
	strcpy(rootEntity->name, "Root");
	rootEntity->prev = nullptr;
	rootEntity->next = nullptr;
	rootEntity->localTransform = transformIdentity();
	rootEntity->worldTransform = transformIdentity();

	meshEntities.reserve(10000);
	meshEntities.add({});

	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool ret;
	{
		SOUL_PROFILE_ZONE_WITH_NAME("Load ASCII From File");
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
	}


	if (!warn.empty())
	{
		SOUL_LOG_WARN("ImportGLTFAssets | %s", warn.c_str());
	}

	if (!err.empty())
	{
		SOUL_LOG_ERROR("ImportGLTFAssets | %s", err.c_str());
		return;
	}

	if (!ret)
	{
		SOUL_LOG_ERROR("ImportGLTFAssets | failed");
		return;
	}

	SOUL_LOG_INFO("Load Textures");
	//Load Textures
	Array<PoolID> textureIDs(&scopeAllocator);
	textureIDs.resize(model.textures.size());
	for (int i = 0; i < model.textures.size(); i++)
	{
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

		GPU::TextureID textureID = gpuSystem->textureCreate(texDesc, (byte*)&image.image[0], image.image.size());

		PoolID sceneTextureID = textures.add({});
		SceneTexture& sceneTexture = textures[sceneTextureID];
		strcpy(sceneTexture.name, texture.name.c_str());
		sceneTexture.rid = textureID;
		textureIDs[i] = sceneTextureID;
	}

	SOUL_LOG_INFO("Load Material");
	//Load Material
	SOUL_LOG_INFO("Materials count : %d", model.materials.size());
	Array<PoolID> materialIDs(&scopeAllocator);
	materialIDs.resize(model.materials.size());
	for (int i = 0; i < model.materials.size(); i++)
	{
		SceneMaterial sceneMaterial = {};
		const tinygltf::Material& material = model.materials[i];

		if (material.values.count("baseColorFactor") != 0)
		{
			const tinygltf::ColorValue& colorValue = material.values.at("baseColorFactor").ColorFactor();
			sceneMaterial.albedo = {
				static_cast<float>(colorValue[0]),
				static_cast<float>(colorValue[1]),
				static_cast<float>(colorValue[2])
			};
		}
		else
		{
			sceneMaterial.albedo = {
				1.0f,
				1.0f,
				1.0f
			};
		}

		if (material.values.count("baseColorTexture") != 0)
		{
			int texID = material.values.at("baseColorTexture").TextureIndex();
			sceneMaterial.albedoTexID = textureIDs[texID];
			sceneMaterial.useAlbedoTex = true;
		}

		if (material.values.count("metallicFactor") != 0)
		{
			sceneMaterial.metallic = material.values.at("metallicFactor").Factor();
		}
		else
		{
			sceneMaterial.metallic = 0.0f;
		}

		if (material.values.count("roughnessFactor") != 0)
		{
			sceneMaterial.roughness = material.values.at("roughnessFactor").Factor();
		}
		else
		{
			sceneMaterial.roughness = 0.0f;
		}

		if (material.values.count("metallicRoughnessTexture") != 0)
		{
			int texID = material.values.at("metallicRoughnessTexture").TextureIndex();

			sceneMaterial.metallicTexID = textureIDs[texID];
			sceneMaterial.metallicTextureChannel = TexChannel::BLUE;
			sceneMaterial.useMetallicTex = true;

			sceneMaterial.roughnessTexID = textureIDs[texID];
			sceneMaterial.roughnessTextureChannel = TexChannel::GREEN;
			sceneMaterial.useRoughnessTex = true;
		}

		if (material.additionalValues.count("normalTexture"))
		{
			int texID = material.additionalValues.at("normalTexture").TextureIndex();

			sceneMaterial.normalTexID = textureIDs[texID];
			sceneMaterial.useNormalTex = true;
		}

		if (material.additionalValues.count("occlusionTexture"))
		{
			int texID = material.additionalValues.at("occlusionTexture").TextureIndex();
			sceneMaterial.aoTexID = textureIDs[texID];
			sceneMaterial.useAOTex = true;
			sceneMaterial.aoTextureChannel = TexChannel::RED;
		}

		if (material.additionalValues.count("emissiveTexture"))
		{
			int texID = material.additionalValues.at("emissiveTexture").TextureIndex();
			sceneMaterial.emissiveTexID = textureIDs[texID];
			sceneMaterial.useEmissiveTex = true;
		}

		if (material.additionalValues.count("emissiveFactor"))
		{
			const tinygltf::ColorValue& colorValue = material.additionalValues.at("emissiveFactor").ColorFactor();
			sceneMaterial.emissive = {
				static_cast<float>(colorValue[0]),
				static_cast<float>(colorValue[1]),
				static_cast<float>(colorValue[2])
			};
		}
		else
		{
			sceneMaterial.emissive = Vec3f(0.0f, 0.0f, 0.0f);
		}

		SOUL_ASSERT(0, strlen(material.name.c_str()) <= 512, "Material name is too long | material.name = %s",
			material.name.c_str());
		strcpy(sceneMaterial.name, material.name.c_str());

		PoolID poolID = materials.add(sceneMaterial);
		materialIDs[i] = poolID;
	}

	struct MaterialData
	{
		Vec3f albedo;
		float metallic;
		Vec3f emissive;
		float roughness;
		uint32 flags;
	};

	enum MaterialFlag
	{
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
	bufferDesc.count = materials.size();
	bufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
	bufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
	materialBuffer = gpuSystem->bufferCreate(
		bufferDesc,
		[this](int index, byte* data)
		{
			auto materialData = (MaterialData*)data;
			SceneMaterial& sceneMaterial = materials[index];

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

			flags |= (MaterialFlag_METALLIC_CHANNEL_RED << uint8(sceneMaterial.metallicTextureChannel));
			flags |= (MaterialFlag_ROUGHNESS_CHANNEL_RED << uint8(sceneMaterial.roughnessTextureChannel));
			flags |= (MaterialFlag_AO_CHANNEL_RED << uint8(sceneMaterial.aoTextureChannel));

			materialData->flags = flags;
		});

	Array<EntityID> entityParents(&scopeAllocator);
	entityParents.reserve(model.nodes.size());
	Array<EntityID> meshEntityIDs(&scopeAllocator);
	meshEntityIDs.reserve(model.meshes.size());

	for (int i = 0; i < model.nodes.size(); i++)
	{
		entityParents.add(rootEntityID);
	}

	for (int i = 0; i < model.meshes.size(); i++)
	{
		meshEntityIDs.add({ 0 });
	}

	EntityID firstEntityID = { 0 };

	SOUL_LOG_INFO("Load Node");
	// LoadNode
	for (int i = 0; i < model.nodes.size(); i++)
	{
		const tinygltf::Node& gltfNode = model.nodes[i];

		EntityID entityID;

		Transform localNodeTransform;
		if (gltfNode.matrix.size() == 16)
		{
			Mat4 temp = mat4(gltfNode.matrix.data());
			localNodeTransform = transformMat4(mat4Transpose(temp));
			localNodeTransform.rotation = unit(localNodeTransform.rotation);
		}
		else
		{
			if (gltfNode.translation.size() == 3)
			{
				localNodeTransform.position = Vec3f(gltfNode.translation[0], gltfNode.translation[1],
					gltfNode.translation[2]);
			}
			else
			{
				localNodeTransform.position = Vec3f(0.0f, 0.0f, 0.0f);
			}
			if (gltfNode.scale.size() == 3)
			{
				localNodeTransform.scale = Vec3f(gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]);
			}
			else
			{
				localNodeTransform.scale = Vec3f(1.0f, 1.0f, 1.0f);
			}
			if (gltfNode.rotation.size() == 4)
			{
				localNodeTransform.rotation = Quaternion(gltfNode.rotation[0], gltfNode.rotation[1],
					gltfNode.rotation[2], gltfNode.rotation[3]);
			}
			else
			{
				localNodeTransform.rotation = quaternionIdentity();
			}
		}

		if (gltfNode.mesh > -1)
		{
			entityID = _createEntity(entityParents[i], EntityType_MESH, gltfNode.name.c_str(),
				localNodeTransform);
			SOUL_ASSERT(0, meshEntityIDs[gltfNode.mesh] == EntityID{ 0 }, "");
			meshEntityIDs[gltfNode.mesh] = entityID;
			SOUL_ASSERT(0, gltfNode.children.size() == 0,
				"Node containing mesh must not have children node. | Mesh index : %d", i);
		}
		else
		{
			entityID = _createEntity(entityParents[i], EntityType_GROUP, gltfNode.name.c_str(),
				localNodeTransform);
		}

		if (i == 0) firstEntityID = entityID;

		for (int j = 0; j < gltfNode.children.size(); j++)
		{
			entityParents[gltfNode.children[j]] = entityID;
		}
	}

	SOUL_LOG_INFO("Load Mesh");
	// Load Mesh
	for (int i = 0; i < model.meshes.size(); i++)
	{
		Memory::ScopeAllocator<> loadMeshAllocator("Load mesh allocator");
		SOUL_MEMORY_ALLOCATOR_ZONE(&loadMeshAllocator);

		struct Vertex
		{
			Vec3f pos;
			Vec3f normal;
			Vec2f texUV;
			Vec3f binormal;
			Vec3f tangent;
		};

		Array<Vertex> vertexes;
		vertexes.reserve(100000);


		MeshEntity* meshEntity = static_cast<MeshEntity*>(_entityPtr(meshEntityIDs[i]));

		const tinygltf::Mesh& mesh = model.meshes[i];

		const tinygltf::Primitive& primitive = mesh.primitives[0];

		const tinygltf::Accessor& positionAccessor = model.accessors[primitive.attributes.at("POSITION")];
		const tinygltf::Accessor& normalAccessor = model.accessors[primitive.attributes.at("NORMAL")];
		const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];

		SOUL_ASSERT(0, positionAccessor.count == normalAccessor.count, "");

		SOUL_ASSERT(0, positionAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT,
			"Component type %d for position is not supported yet. | mesh name = %s.",
			positionAccessor.componentType, mesh.name.c_str());
		SOUL_ASSERT(0, positionAccessor.type == TINYGLTF_TYPE_VEC3,
			"Type %d for position is not supported yet. | mesh name = %s.", positionAccessor.type,
			mesh.name.c_str());
		const tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccessor.bufferView];
		int positionOffset = positionAccessor.byteOffset + positionBufferView.byteOffset;
		int positionByteStride = positionAccessor.ByteStride(positionBufferView);
		unsigned char* positionBuffer = &model.buffers[positionBufferView.buffer].data[positionOffset];

		SOUL_ASSERT(0, normalAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT,
			"Component type %d for normal is not supported yet. | mesh name = %s.",
			normalAccessor.componentType, mesh.name.c_str());
		SOUL_ASSERT(0, normalAccessor.type == TINYGLTF_TYPE_VEC3,
			"Type %d for normal is not supported yet. | mesh name = %s.", normalAccessor.type,
			mesh.name.c_str());
		const tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccessor.bufferView];
		int normalOffset = normalAccessor.byteOffset + normalBufferView.byteOffset;
		int normalByteStride = normalAccessor.ByteStride(normalBufferView);
		unsigned char* normalBuffer = &model.buffers[normalBufferView.buffer].data[normalOffset];

		AABB meshAABB;
		Vec3f position0 = *(Vec3f*)&positionBuffer[0];
		meshAABB.min = meshEntity->worldTransform * position0;
		meshAABB.max = meshAABB.min;

		for (int i = 1; i < positionAccessor.count; i++)
		{
			Vec3f position = *(Vec3f*)&positionBuffer[positionByteStride * i];

			Vec3f worldPosition = meshEntity->worldTransform * position;

			meshAABB.min = componentMin(meshAABB.min, worldPosition);
			meshAABB.max = componentMax(meshAABB.max, worldPosition);
		}

		Mat4 vertexPositionTransform = mat4Identity();

		bool positionToAABBCenter = true;
		if (positionToAABBCenter)
		{
			vertexPositionTransform = mat4Transform(meshEntity->worldTransform);
			Vec3f meshAABBCenter = (meshAABB.min + meshAABB.max) / 2.0f;
			meshEntity->worldTransform.position = meshAABBCenter;
			Mat4 localMat4 = mat4Inverse(mat4Transform(meshEntity->parent->worldTransform)) * mat4Transform(
				meshEntity->worldTransform);
			meshEntity->localTransform = transformMat4(localMat4);
			vertexPositionTransform = mat4Inverse(mat4Transform(meshEntity->worldTransform)) * vertexPositionTransform;
		}

		Array<Vec2f> texCoord0s;
		texCoord0s.reserve(positionAccessor.count);
		if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
		{
			const tinygltf::Accessor& texCoord0Accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
			SOUL_ASSERT(0, texCoord0Accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT,
				"Component type %d for texCoord0 is not supported yet. | mesh name = %s.",
				texCoord0Accessor.componentType, mesh.name.c_str());
			SOUL_ASSERT(0, texCoord0Accessor.type == TINYGLTF_TYPE_VEC2,
				"Type %d for texCoord0 is not supported yet. | mesh name = %s.", texCoord0Accessor.type,
				mesh.name.c_str());
			const tinygltf::BufferView& texCoord0BufferView = model.bufferViews[texCoord0Accessor.bufferView];
			int texCoord0Offset = texCoord0Accessor.byteOffset + texCoord0BufferView.byteOffset;
			int texCoord0ByteStride = texCoord0Accessor.ByteStride(texCoord0BufferView);
			unsigned char* texCoord0Buffer = &model.buffers[texCoord0BufferView.buffer].data[texCoord0Offset];
			for (int i = 0; i < positionAccessor.count; i++)
			{
				texCoord0s.add(*(Vec2f*)&texCoord0Buffer[texCoord0ByteStride * i]);
			}
		}
		else
		{
			for (int i = 0; i < positionAccessor.count; i++) {
				texCoord0s.add(Vec2f(0.0f, 0.0f));
			}
		}

		Array<Vec4f> tangents;
		tangents.reserve(positionAccessor.count);
		if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
		{
			const tinygltf::Accessor& tangentAccessor = model.accessors[primitive.attributes.at("TANGENT")];
			SOUL_ASSERT(0, tangentAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT,
				"Component type %d for tangent is not supported yet. | mesh name = %s.",
				tangentAccessor.componentType, mesh.name.c_str());
			SOUL_ASSERT(0, tangentAccessor.type == TINYGLTF_TYPE_VEC4,
				"Type %d for tangent is not supported yet. | mesh name = %s.", tangentAccessor.type,
				mesh.name.c_str());
			const tinygltf::BufferView& tangentBufferView = model.bufferViews[tangentAccessor.bufferView];
			int tangentOffset = tangentAccessor.byteOffset + tangentBufferView.byteOffset;
			int tangentByteStride = tangentAccessor.ByteStride(tangentBufferView);
			unsigned char* tangentBuffer = &model.buffers[tangentBufferView.buffer].data[tangentOffset];
			for (int i = 0; i < positionAccessor.count; i++)
			{
				tangents.add(*(Vec4f*)&tangentBuffer[tangentByteStride * i]);
			}
		}
		else
		{
			for (int i = 0; i < positionAccessor.count; i++)
			{
				tangents.add(Vec4f(0.0f, 1.0f, 0.0f, 1.0f));
			}
		}

		for (int i = 0; i < positionAccessor.count; i++)
		{
			Vec3f position = *(Vec3f*)&positionBuffer[positionByteStride * i];
			Vec3f normal = *(Vec3f*)&normalBuffer[normalByteStride * i];
			Vec4f tangent = tangents[i];
			Vec2f texCoord0 = texCoord0s[i];

			vertexes.add({
				vertexPositionTransform * position,
				normal,
				texCoord0,
				cross(normal, tangent.xyz()),
				tangent.xyz()
				});
		}

		SOUL_ASSERT(0, indexAccessor.type == TINYGLTF_TYPE_SCALAR,
			"Type %d for index is not supported. | mesh name = %s.", indexAccessor.type, mesh.name.c_str());
		const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
		int indexOffset = indexAccessor.byteOffset + indexBufferView.byteOffset;
		int indexByteStride = indexAccessor.ByteStride(indexBufferView);
		unsigned char* indexBuffer = &model.buffers[indexBufferView.buffer].data[indexOffset];

		Mesh sceneMesh = {};

		GPU::BufferDesc vertexBufferDesc;
		vertexBufferDesc.typeSize = sizeof(Vertex);
		vertexBufferDesc.typeAlignment = alignof(Vertex);
		vertexBufferDesc.count = vertexes.size();
		vertexBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		vertexBufferDesc.usageFlags = GPU::BUFFER_USAGE_VERTEX_BIT;
		sceneMesh.vertexBufferID = gpuSystem->bufferCreate(vertexBufferDesc,
			[&vertexes](int index, byte* data)
			{
				const auto vertex = (Vertex*)data;
				(*vertex) = vertexes[index];
			});


		GPU::BufferDesc indexBufferDesc;
		indexBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		indexBufferDesc.usageFlags = GPU::BUFFER_USAGE_INDEX_BIT;

		if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
		{
			using Index = uint32;
			Array<Index> indexes;
			indexes.reserve(100000);
			for (int i = 0; i < indexAccessor.count; ++i)
			{
				uint32 index = *(uint32*)&indexBuffer[indexByteStride * i];
				indexes.add(index);
			}
			indexBufferDesc.typeSize = sizeof(Index);
			indexBufferDesc.typeAlignment = alignof(Index);
			indexBufferDesc.count = indexes.size();

			sceneMesh.indexBufferID = gpuSystem->bufferCreate(indexBufferDesc,
				[&indexes](int i, byte* data)
				{
					const auto index = (Index*)data;
					(*index) = indexes[i];
				});
			sceneMesh.indexCount = indexes.size();
		}
		else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
		{

			using Index = uint16;
			Array<Index> indexes;
			indexes.reserve(100000);
			for (int i = 0; i < indexAccessor.count; ++i)
			{
				uint32 index = *(uint32*)&indexBuffer[indexByteStride * i];
				indexes.add(index);
			}
			indexBufferDesc.typeSize = sizeof(Index);
			indexBufferDesc.typeAlignment = alignof(Index);
			indexBufferDesc.count = indexes.size();

			sceneMesh.indexBufferID = gpuSystem->bufferCreate(indexBufferDesc,
				[&indexes](int i, byte* data)
				{
					const auto index = (Index*)data;
					(*index) = indexes[i];
				});
			sceneMesh.indexCount = indexes.size();
			SOUL_LOG_INFO("Index buffer id = %d, count = %d, bytes = %d", sceneMesh.indexBufferID, indexes.size(), indexes.size() * sizeof(Index));
		}
		else
		{
			SOUL_NOT_IMPLEMENTED();
		}

		meshEntity->meshID = meshes.add(sceneMesh);
		meshEntity->materialID = materialIDs[primitive.material];
	}

	Vec2ui32 sceneResolution = { camera.viewportWidth, camera.viewportHeight };
	camera.position = Vec3f(1.0f, 1.0f, 0.0f);
	camera.direction = Vec3f(0.0f, 0.0f, -1.0f);
	camera.up = Vec3f(0.0f, 1.0f, 0.0f);
	camera.perspective.fov = PI / 4;
	camera.perspective.aspectRatio = sceneResolution.x / sceneResolution.y;
	camera.perspective.zNear = 0.1f;
	camera.perspective.zFar = 30.0f;
	camera.projection = mat4Perspective(
		camera.perspective.fov,
		camera.perspective.aspectRatio,
		camera.perspective.zNear,
		camera.perspective.zFar);
}

bool DeferredPipeline::Scene::handleInput() {
	ImGuiIO& io = ImGui::GetIO();
	if (ImGui::IsMouseDown(2)) {
		static float translationSpeed = 1.0f;

		float cameraSpeedInc = 0.1f;
		translationSpeed += (cameraSpeedInc * translationSpeed * io.MouseWheel);

		if (ImGui::IsKeyPressed(GLFW_KEY_M)) {
			translationSpeed *= 0.9f;
		}
		if (ImGui::IsKeyPressed(GLFW_KEY_N)) {
			translationSpeed *= 1.1;
		}

		if (ImGui::IsMouseDragging(2)) {

			Soul::Vec3f cameraRight = cross(camera.up, camera.direction) * -1.0f;

			{
				Soul::Mat4 rotate = mat4Rotate(cameraRight, -2.0f * io.MouseDelta.y / camera.viewportHeight * Soul::PI);
				camera.direction = rotate * camera.direction;
				camera.up = rotate * camera.up;
			}

			{
				Soul::Mat4 rotate = mat4Rotate(Soul::Vec3f(0.0f, 1.0f, 0.0f), -2.0f * io.MouseDelta.x / camera.viewportWidth * Soul::PI);

				if (camera.direction != Soul::Vec3f(0.0f, 1.0f, 0.0f))
					camera.direction = rotate * camera.direction;
				if (camera.up != Soul::Vec3f(0.0f, 1.0f, 0.0f))
					camera.up = rotate * camera.up;
			}
		}

		Soul::Vec3f right = Soul::unit(cross(camera.direction, camera.up));
		if (ImGui::IsKeyPressed(GLFW_KEY_W)) {
			camera.position += Soul::unit(camera.direction) * translationSpeed;
		}
		if (ImGui::IsKeyPressed(GLFW_KEY_S)) {
			camera.position -= Soul::unit(camera.direction) * translationSpeed;
		}
		if (ImGui::IsKeyPressed(GLFW_KEY_A)) {
			camera.position -= Soul::unit(right) * translationSpeed;
		}
		if (ImGui::IsKeyPressed(GLFW_KEY_D)) {
			camera.position += Soul::unit(right) * translationSpeed;
		}
		return true;
	}

	camera.view = mat4View(camera.position, camera.position + camera.direction, camera.up);
	dirLight.updateShadowMatrixes(camera);
	return false;
}

DeferredPipeline::EntityID DeferredPipeline::Scene::_createEntity(EntityID parentID, EntityType entityType, const char* name, Transform localTransform) {
	EntityID entityID;
	entityID.type = entityType;
	Entity* entity = nullptr;
	switch (entityType)
	{
	case EntityType_MESH:
		entityID.index = meshEntities.add(MeshEntity());
		entity = meshEntities.ptr(entityID.index);
		break;
	case EntityType_GROUP:
		entityID.index = groupEntities.add(GroupEntity());
		entity = groupEntities.ptr(entityID.index);
		break;
	default:
		SOUL_ASSERT(0, false, "Entity type is not valid, entity type = %d", entityType);
	}

	GroupEntity* parent = static_cast<GroupEntity*>(_entityPtr(parentID));
	Entity* next = parent->first;
	if (next != nullptr)
	{
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

void DeferredPipeline::Scene::cleanup() {
	for (Mesh& mesh : meshes) {
		gpuSystem->bufferDestroy(mesh.indexBufferID);
		gpuSystem->bufferDestroy(mesh.vertexBufferID);
	}

	if (materialBuffer != GPU::BUFFER_ID_NULL) {
		gpuSystem->bufferDestroy(materialBuffer);
	}

	for (SceneTexture& texture : textures) {
		gpuSystem->textureDestroy(texture.rid);
	}

	groupEntities.cleanup();
	meshEntities.cleanup();
	meshes.cleanup();
	materials.cleanup();
	textures.cleanup();
}

DeferredPipeline::Entity* DeferredPipeline::Scene::_entityPtr(EntityID entityID) {
	switch (entityID.type)
	{
	case EntityType_MESH:
		return meshEntities.ptr(entityID.index);
	case EntityType_GROUP:
		return groupEntities.ptr(entityID.index);
	default:
		SOUL_ASSERT(0, false, "Entity type is not valid, entity type = %d", entityID.type);
		return nullptr;
	}
}