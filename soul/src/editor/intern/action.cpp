#include "core/type.h"
#include "core/debug.h"
#include "core/math.h"

#include "editor/intern/entity.h"
#include "editor/intern/action.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include "extern/tiny_gltf.h"

namespace Soul {
	namespace Editor {

		EntityID ActionImportGLTFAsset(World* world, const char* path, bool positionToAABBCenter) {
			tinygltf::Model model;
			tinygltf::TinyGLTF loader;
			std::string err;
			std::string warn;
			bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);

			Render::System& renderSystem = world->renderSystem;

			if (!warn.empty()) {
				SOUL_LOG_WARN("ImportGLTFAssets | %s", warn.c_str());
			}

			if (!err.empty()) {
				SOUL_LOG_ERROR("ImportGLTFAssets | %s", err.c_str());
				return { 0 };
			}

			if (!ret) {
				SOUL_LOG_ERROR("ImportGLTFAssets | failed");
				return { 0 };
			}

			Array<PoolID> textureIDs;
			textureIDs.resize(model.textures.size());

			//Load Textures
			for (int i = 0; i < model.textures.size(); i++) {
				const tinygltf::Texture& texture = model.textures[i];
				const tinygltf::Image& image = model.images[texture.source];

				Soul::Render::TexSpec texSpec;

				texSpec.pixelFormat = Soul::Render::PixelFormat_RGBA;
				texSpec.filterMin = Soul::Render::TexFilter_LINEAR_MIPMAP_LINEAR;
				texSpec.filterMag = Soul::Render::TexFilter_LINEAR;
				texSpec.wrapS = Soul::Render::TexWrap_REPEAT;
				texSpec.wrapT = Soul::Render::TexWrap_REPEAT;

				texSpec.width = image.width;
				texSpec.height = image.height;

				Soul::Render::MaterialRID textureRID = renderSystem.textureCreate(texSpec, (unsigned char*)&image.image[0], image.component);

				Texture editorTexture = { 0 };
				strcpy(editorTexture.name, texture.name.c_str());
				editorTexture.rid = textureRID;
				PoolID poolID = world->textures.add(editorTexture);

				textureIDs[i] = poolID;

			}

			Array<PoolID> materialIDs;
			materialIDs.resize(model.materials.size());

			//Load Material
			for (int i = 0; i < model.materials.size(); i++) {
				Material editorMaterial = { 0 };
				const tinygltf::Material material = model.materials[i];

				if (material.values.count("baseColorFactor") != 0) {
					const tinygltf::ColorValue& colorValue = material.values.at("baseColorFactor").ColorFactor();
					editorMaterial.albedo = {
						(float)colorValue[0],
						(float)colorValue[1],
						(float)colorValue[2]
					};
				}

				if (material.values.count("baseColorTexture") != 0) {
					int texID = material.values.at("baseColorTexture").TextureIndex();
					editorMaterial.albedoTexID = textureIDs[texID];
					editorMaterial.useAlbedoTex = true;
				}

				if (material.values.count("metallicFactor") != 0) {
					editorMaterial.metallic = material.values.at("metallicFactor").Factor();
				}
				else {
					editorMaterial.metallic = 0.0f;
				}

				if (material.values.count("roughnessFactor") != 0) {
					editorMaterial.roughness = material.values.at("roughnessFactor").Factor();
				}
				else {
					editorMaterial.roughness = 0.0f;
				}

				if (material.values.count("metallicRoughnessTexture") != 0) {
					int texID = material.values.at("metallicRoughnessTexture").TextureIndex();

					editorMaterial.metallicTexID = textureIDs[texID];
					editorMaterial.metallicTextureChannel = Render::TexChannel_BLUE;
					editorMaterial.useMetallicTex = true;

					editorMaterial.roughnessTexID = textureIDs[texID];
					editorMaterial.roughnessTextureChannel = Render::TexChannel_GREEN;
					editorMaterial.useRoughnessTex = true;
				}

				if (material.additionalValues.count("normalTexture")) {
					int texID = material.additionalValues.at("normalTexture").TextureIndex();

					editorMaterial.normalTexID = textureIDs[texID];
					editorMaterial.useNormalTex = true;

				}

				if (material.additionalValues.count("occlusionTexture")) {
					int texID = material.additionalValues.at("occlusionTexture").TextureIndex();
					editorMaterial.aoTexID = textureIDs[texID];
					editorMaterial.useAOTex = true;
					editorMaterial.aoTextureChannel = Render::TexChannel_RED;
				}

				if (material.additionalValues.count("emissiveTexture")) {
					int texID = material.additionalValues.at("emissiveTexture").TextureIndex();
					editorMaterial.emissiveTexID = textureIDs[texID];
					editorMaterial.useEmissiveTex = true;
				}

				if (material.additionalValues.count("emissiveFactor")) {
					const tinygltf::ColorValue& colorValue = material.additionalValues.at("emissiveFactor").ColorFactor();
					editorMaterial.emissive = {
						(float)colorValue[0],
						(float)colorValue[1],
						(float)colorValue[2]
					};
				}
				else {
					editorMaterial.emissive = Vec3f(0.0f, 0.0f, 0.0f);
				}

				SOUL_ASSERT(0, strlen(material.name.c_str()) <= 512, "Material name is too long | material.name = %s", material.name.c_str());
				strcpy(editorMaterial.name, material.name.c_str());

				PoolArray<Texture>& textures = world->textures;

				editorMaterial.rid = renderSystem.materialCreate({
					textures[editorMaterial.albedoTexID].rid,
					textures[editorMaterial.normalTexID].rid,
					textures[editorMaterial.metallicTexID].rid,
					textures[editorMaterial.roughnessTexID].rid,
					textures[editorMaterial.aoTexID].rid,
					textures[editorMaterial.emissiveTexID].rid,

					editorMaterial.useAlbedoTex,
					editorMaterial.useNormalTex,
					editorMaterial.useMetallicTex,
					editorMaterial.useRoughnessTex,
					editorMaterial.useAOTex,
					editorMaterial.useEmissiveTex,

					editorMaterial.albedo,
					editorMaterial.metallic,
					editorMaterial.roughness,
					editorMaterial.emissive,

					editorMaterial.metallicTextureChannel,
					editorMaterial.roughnessTextureChannel,
					editorMaterial.aoTextureChannel
				});

				PoolID poolID = world->materials.add(editorMaterial);
				materialIDs[i] = poolID;
			}


			Array<EntityID> entityParents;
			entityParents.reserve(model.nodes.size());
			Array<EntityID> meshEntityIDs;
			meshEntityIDs.reserve(model.meshes.size());

			for (int i = 0; i < model.nodes.size(); i++) {
				entityParents.add(world->rootEntityID);
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
					entityID = EntityCreate(world, entityParents[i], EntityType_MESH, gltfNode.name.c_str(), localNodeTransform);
					meshEntityIDs[gltfNode.mesh] = entityID;
					SOUL_ASSERT(0, gltfNode.children.size() == 0, "Node containing mesh must not have children node. | Mesh index : %d", i);
				}
				else {
					entityID = EntityCreate(world, entityParents[i], EntityType_GROUP, gltfNode.name.c_str(), localNodeTransform);
				}

				if (i == 0) firstEntityID = entityID;

				for (int j = 0; j < gltfNode.children.size(); j++) {
					entityParents[gltfNode.children[j]] = entityID;
				}

			}

			entityParents.cleanup();

			// Load Mesh
			for (int i = 0; i < model.meshes.size(); i++) {

				Soul::Array<Soul::Render::Vertex> vertexes;
				vertexes.reserve(100000);
				Soul::Array<uint32> indexes;
				indexes.reserve(100000);

				MeshEntity *meshEntity = (MeshEntity*)EntityPtr(world, meshEntityIDs[i]);

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

				PoolArray<Material>& materials = world->materials;

				meshEntity->meshRID = renderSystem.meshCreate({
					mat4Transform(meshEntity->worldTransform),
					vertexes._buffer,
					indexes._buffer,
					(uint32)vertexes.size(),
					(uint32)indexes.size(),
					materials[materialIDs[primitive.material]].rid
				});
				meshEntity->materialID = materialIDs[primitive.material];

				texCoord0s.cleanup();
				tangents.cleanup();
				vertexes.cleanup();
				indexes.cleanup();
			}

			meshEntityIDs.cleanup();
			textureIDs.cleanup();
			materialIDs.cleanup();

			return firstEntityID;
		}
	}
}