#pragma once

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "extern/tiny_obj_loader.h"
#include "extern/stb_image.h"
#include "extern/tiny_gltf.h"

#include "render/system.h"
#include "render/data.h"
#include "core/array.h"
#include "core/type.h"
#include "core/debug.h"
#include "core/math.h"
#include "sandbox/type.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

struct _VertexBuffer {
	Soul::Array<Soul::Vec3f> position;
	Soul::Array<Soul::Vec3f> normal;
	Soul::Array<Soul::Vec2f> texCoord;
};

struct _Index {
	int v;
	int vn;
	int vt;

	bool operator==(const _Index& other) const
	{
		return (v == other.v) && (vn == other.vn) && (vt == other.vt);
	}

	bool operator<(const _Index& other) const
	{
		if (v < other.v) return true;
		if (v == other.v && vn < other.vn) return true;
		if (v == other.v && vn == other.vn && vt < other.vt) return true;

		return false;
	}

	std::string toString() const
	{
		std::string res;
		res.reserve(100);
		
		res += std::to_string(v);
		res += "|";
		res += std::to_string(vn);
		res += "|";
		res += std::to_string(vt);

		return res;
	}
};

struct _CallbackData {
	_VertexBuffer vertexBuffer;
	Soul::Array<_Index> indexBuffer;
	Soul::Array<int> indexCountBuffer;
	std::vector<tinyobj::material_t> materials;
	Soul::Array<int> materialIndexes;
	Soul::Array<int> materialStartIndexes;
};

void _VertexCallback(void* userData, float x, float y, float z, float w) {
	_CallbackData* callbackData = (_CallbackData*)userData;
	callbackData->vertexBuffer.position.pushBack(Soul::Vec3f(x, y, z));
}

void _NormalCallback(void* userData, float x, float y, float z) {
	_CallbackData* callbackData = (_CallbackData*)userData;
	callbackData->vertexBuffer.normal.pushBack(Soul::Vec3f(x, y, z));
}

void _TexCoordCallback(void* userData, float x, float y, float z) {
	_CallbackData* callbackData = (_CallbackData*)userData;
	callbackData->vertexBuffer.texCoord.pushBack(Soul::Vec2f(x, y));
}

void _IndexCallback(void* userData, tinyobj::index_t *indices, int indexCount) {
	_CallbackData* callbackData = (_CallbackData*)userData;
	SOUL_ASSERT(0, callbackData->materialIndexes.getSize() != 0, "There is faces with no material specification");
	SOUL_ASSERT(0, indexCount >= 3, "There is faces with less than three indexes");

	callbackData->indexCountBuffer.pushBack(indexCount);

	for (int i = 0; i < indexCount; i++) {
		SOUL_ASSERT(0, indices[i].vertex_index > 0, "There is index without reference to vertex");
		SOUL_ASSERT(0, indices[i].normal_index > 0, "There is index without reference to normal");
		SOUL_ASSERT(0, indices[i].texcoord_index > 0, "There is index without reference to texcoord");
		callbackData->indexBuffer.pushBack({ indices[i].vertex_index, indices[i].normal_index, indices[i].texcoord_index });
	}
	
	/* for (int i = 0; i < 3; i++) {
		callbackData->indexBuffer.pushBack({ indices[i].vertex_index, indices[i].normal_index, indices[i].texcoord_index });
	}

	for (int i = 3; i < indexCount; i++) {
		int offset[3] = { 3, 1, 0 };
		for (int j = 0; j < 3; j++) {
			tinyobj::index_t idx = indices[i - offset[j]];
			callbackData->indexBuffer.pushBack({ idx.vertex_index, idx.normal_index, idx.texcoord_index });
		}
	}*/
}

void _MTLLibCallback(void* userData, const tinyobj::material_t* materials, int materialCount) {
	_CallbackData* callbackData = (_CallbackData*)userData;
	for (int i = 0; i < materialCount; i++) {
		const tinyobj::material_t& material = materials[i];
		callbackData->materials.push_back(material);
	}
}

void _UseMTLCallback(void* userData, const char* name, int materialIdx) {
	_CallbackData* callbackData = (_CallbackData*)userData;
	// SOUL_ASSERT(0, materialIdx != -1, "Material not found| name = %s", name);
	callbackData->materialIndexes.pushBack(materialIdx);
	callbackData->materialStartIndexes.pushBack(callbackData->indexCountBuffer.getSize());
}

void ImportObjMtlAssets(
	SceneData* sceneData, 
	const char* objFilePath, 
	const char* assetDir) {
	
	Soul::Render::System* renderSystem = &sceneData->renderSystem;

	std::ifstream ifs(objFilePath);
	SOUL_ASSERT(0, !ifs.fail(), "Failed to load .obj file|objFilePath = %s", objFilePath);

	tinyobj::MaterialFileReader mtlReader(assetDir);

	tinyobj::callback_t cb;
	cb.vertex_cb = _VertexCallback;
	cb.normal_cb = _NormalCallback;
	cb.texcoord_cb = _TexCoordCallback;
	cb.index_cb = _IndexCallback;
	cb.mtllib_cb = _MTLLibCallback;
	cb.usemtl_cb = _UseMTLCallback;
	cb.group_cb = nullptr;
	cb.object_cb = nullptr;

	std::string warn;
	std::string err;

	_CallbackData callbackData;
	callbackData.vertexBuffer.position.init(500000);
	callbackData.vertexBuffer.normal.init(500000);
	callbackData.vertexBuffer.texCoord.init(500000);
	callbackData.indexCountBuffer.init(500000);
	callbackData.indexBuffer.init(500000);
	callbackData.materialIndexes.init(500000);
	callbackData.materialStartIndexes.init(500000);

	bool ret = tinyobj::LoadObjWithCallback(ifs, cb, &callbackData, &mtlReader, &warn, &err);

	SOUL_ASSERT(0, err.empty(), "There is error when parsing .obj file|error = %s", err);
	SOUL_ASSERT(0, ret == true, "Failed to parse .obj file");

	SOUL_ASSERT(0, callbackData.vertexBuffer.position.getSize() >= 0, "There is no vertex position information");
	SOUL_ASSERT(0, callbackData.vertexBuffer.normal.getSize() >= 0, "There is no normal information");
	SOUL_ASSERT(0, callbackData.vertexBuffer.texCoord.getSize() >= 0, "There is no texcoord information");
	SOUL_ASSERT(0, callbackData.indexCountBuffer.getSize() >= 0, "There is no index count information");
	SOUL_ASSERT(0, callbackData.indexBuffer.getSize() >= 0, "There is no index information");
	SOUL_ASSERT(0, callbackData.materialIndexes.getSize() >= 0, "There is no material index information");
	SOUL_ASSERT(0, callbackData.materialIndexes.getSize() == callbackData.materialStartIndexes.getSize(),
		"materialIndexes and materialStartIndexes not the same size");

	stbi_set_flip_vertically_on_load(true);

	int baseIndex = 0;

	Soul::Array<Soul::Render::RID> texIDMapping;
	texIDMapping.init(callbackData.materials.size());
	for (int i = 0; i < callbackData.materials.size(); i++) {
		
		tinyobj::material_t material = callbackData.materials[i];

		std::string albedoFilename = std::string(assetDir) + material.diffuse_texname;
		std::string metallicFilename = std::string(assetDir) + material.metallic_texname;
		std::string roughnessFilename = std::string(assetDir) + material.roughness_texname;
		std::string normalFilename = std::string(assetDir) + material.normal_texname;

		UITexture sceneMetallicTexture;
		strcpy(sceneMetallicTexture.name, material.metallic_texname.c_str());

		Soul::Render::TexSpec texSpec;
		texSpec.pixelFormat = Soul::Render::PixelFormat_RGBA;
		texSpec.filterMin = Soul::Render::TexFilter_LINEAR_MIPMAP_LINEAR;
		texSpec.filterMag = Soul::Render::TexFilter_LINEAR;
		texSpec.wrapS = Soul::Render::TexWrap_REPEAT;
		texSpec.wrapT = Soul::Render::TexWrap_REPEAT;
		int numChannel;

		uint32 albedoTexID = 0;
		UITexture sceneAlbedoTex;
		strcpy(sceneAlbedoTex.name, material.diffuse_texname.c_str());
		unsigned char* albedoRaw = stbi_load(albedoFilename.c_str(),
			&texSpec.width, &texSpec.height, &numChannel, 0);
		SOUL_ASSERT(0, albedoRaw != nullptr, "Albedo file does not exist| filepath = %s", albedoFilename);
		sceneAlbedoTex.rid = renderSystem->textureCreate(texSpec, albedoRaw, numChannel);
		sceneData->textures.pushBack(sceneAlbedoTex);
		albedoTexID = sceneData->textures.getSize() - 1;
		stbi_image_free(albedoRaw);

		uint32 metallicTexID = 0;
		UITexture sceneMetallicTex;
		strcpy(sceneMetallicTex.name, material.metallic_texname.c_str());
		unsigned char* metallicRaw = stbi_load(metallicFilename.c_str(),
			&texSpec.width, &texSpec.height, &numChannel, 0);
		SOUL_ASSERT(0, metallicRaw != nullptr, "Metallic file does not exist| filepath = %s", metallicFilename);
		sceneMetallicTex.rid = renderSystem->textureCreate(texSpec, metallicRaw, numChannel);
		sceneData->textures.pushBack(sceneMetallicTex);
		metallicTexID = sceneData->textures.getSize() - 1;
		stbi_image_free(metallicRaw);
		

		uint32 roughnessTexID = 0;
		UITexture sceneRoughnessTex;
		strcpy(sceneRoughnessTex.name, material.roughness_texname.c_str());
		unsigned char* roughnessRaw = stbi_load(roughnessFilename.c_str(),
			&texSpec.width, &texSpec.height, &numChannel, 0);
		SOUL_ASSERT(0, roughnessRaw != nullptr, "Roughness file does not exist| filepath = %s", roughnessFilename);
		sceneRoughnessTex.rid = renderSystem->textureCreate(texSpec, roughnessRaw, numChannel);
		sceneData->textures.pushBack(sceneRoughnessTex);
		roughnessTexID = sceneData->textures.getSize() - 1;
		stbi_image_free(roughnessRaw);


		uint32 normalTexID = 0;
		UITexture sceneNormalTex;
		strcpy(sceneNormalTex.name, material.normal_texname.c_str());
		unsigned char* normalRaw = stbi_load(normalFilename.c_str(),
			&texSpec.width, &texSpec.height, &numChannel, 0);
		SOUL_ASSERT(0, normalRaw != nullptr, "Normal file does not exist| filepath = %s", normalFilename);
		sceneNormalTex.rid = renderSystem->textureCreate(texSpec, normalRaw, numChannel);
		sceneData->textures.pushBack(sceneNormalTex);
		normalTexID = sceneData->textures.getSize() - 1;
		stbi_image_free(normalRaw);

		Soul::Render::MaterialSpec materialSpec = {
			sceneData->textures[albedoTexID].rid,
			sceneData->textures[normalTexID].rid,
			sceneData->textures[metallicTexID].rid,
			sceneData->textures[roughnessTexID].rid,
			0,

			true,
			true,
			true,
			true,
			false,

			Soul::Vec3f(0.0f, 0.0f, 0.0f),
			0.0f, 
			0.0f,

			Soul::Render::TexChannel_RED,
			Soul::Render::TexChannel_RED,
			Soul::Render::TexChannel_RED,
		
};

		Soul::Render::RID materialRID = renderSystem->materialCreate(materialSpec);
		
		UIMaterial sceneMaterial = { 0 };
		strcpy(sceneMaterial.name, material.name.c_str());

		sceneMaterial.rid = materialRID;

		sceneMaterial.albedoTexID = albedoTexID;
		sceneMaterial.useAlbedoTex = true;
		
		sceneMaterial.normalTexID = normalTexID;
		sceneMaterial.useNormalTex = true;
		
		sceneMaterial.metallicTexID = metallicTexID;
		sceneMaterial.useMetallicTex = true;

		sceneMaterial.roughnessTexID = roughnessTexID;
		sceneMaterial.useRoughnessTex = true;

		sceneData->materials.pushBack(sceneMaterial);

		texIDMapping[i] = sceneData->materials.getSize() - 1;
	}

	for (int i = 0; i < callbackData.materialIndexes.getSize(); i++) {

		int start = callbackData.materialStartIndexes.get(i);
		int end = i == callbackData.materialIndexes.getSize() - 1 ? callbackData.indexCountBuffer.getSize() - 1 : callbackData.materialStartIndexes.get(i + 1) - 1;
		int materialIndex = callbackData.materialIndexes[i];

		if (materialIndex == -1)
		{
			for (int j = start; j <= end; j++) {
				baseIndex += callbackData.indexCountBuffer[j];
			}
			continue;
		}
		
		Soul::Array<Soul::Render::Vertex> vertexes;
		vertexes.init(100000);
		Soul::Array<uint32> indexes;
		indexes.init(100000);
		
		for (int j = start; j <= end; j++) {

			SOUL_ASSERT(0, j < callbackData.indexCountBuffer.getSize(), "");
			int baseVertex = vertexes.getSize();
			int indexCount = callbackData.indexCountBuffer[j];

			for (int k = 0; k < callbackData.indexCountBuffer[j]; k++) {
				_Index& index = callbackData.indexBuffer[baseIndex + k];
				Soul::Vec3f pos = callbackData.vertexBuffer.position.get(index.v - 1);
				Soul::Vec3f normal = callbackData.vertexBuffer.normal.get(index.vn - 1);
				Soul::Vec2f texCoord = callbackData.vertexBuffer.texCoord.get(index.vt - 1);
				vertexes.pushBack({
					pos,
					normal,
					texCoord,
					Soul::Vec3f(0.0f, 0.0f, 0.0f),
					Soul::Vec3f(0.0f, 0.0f, 0.0f)
				});
			}

			for (int k = 0; k < 3; k++) {
				indexes.pushBack(baseVertex + k);
			}

			for (int k = 3; k < callbackData.indexCountBuffer[j]; k++) {
				int offset[3] = { k, 1, 0 };
				for (int l = 0; l < 3; l++) {
					indexes.pushBack(baseVertex + k - offset[l]);
				}
			}

			baseIndex += indexCount;
		}

		for (int j = 0; j < indexes.getSize(); j+=3) {
			Soul::Render::Vertex& vertex1 = vertexes.buffer[indexes.get(j)];
			Soul::Render::Vertex& vertex2 = vertexes.buffer[indexes.get(j + 1)];
			Soul::Render::Vertex& vertex3 = vertexes.buffer[indexes.get(j + 2)];

			Soul::Vec3f edge1 = vertex2.pos - vertex1.pos;
			Soul::Vec3f edge2 = vertex3.pos - vertex1.pos;

			float deltaU1 = vertex2.texUV.x - vertex1.texUV.x;
			float deltaV1 = vertex2.texUV.y - vertex1.texUV.y;
			float deltaU2 = vertex3.texUV.x - vertex1.texUV.x;
			float deltaV2 = vertex3.texUV.y - vertex1.texUV.y;

			float f = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);

			Soul::Vec3f tangent;

			tangent.x = f * (deltaV2 * edge1.x - deltaV1 * edge2.x);
			tangent.y = f * (deltaV2 * edge1.y - deltaV1 * edge2.y);
			tangent.z = f * (deltaV2 * edge1.z - deltaV1 * edge2.z);

			vertex1.tangent += tangent;
			vertex2.tangent += tangent;
			vertex3.tangent += tangent;

		}

		for (int j = 0; j < vertexes.getSize(); j++) {
			vertexes.buffer[j].normal = Soul::unit(vertexes.buffer[j].normal);
			vertexes.buffer[j].tangent = Soul::unit(vertexes.buffer[j].tangent);
			vertexes.buffer[j].binormal = Soul::cross(vertexes.buffer[j].normal, vertexes.buffer[j].tangent);
			vertexes.buffer[j].binormal = Soul::unit(vertexes.buffer[j].binormal);
		}

		uint32 materialID = texIDMapping[materialIndex];
		Soul::Render::RID materialRID = sceneData->materials[materialID].rid;
		Soul::Render::MeshSpec meshSpec = {
				Soul::mat4Identity(),
				vertexes.buffer,
				indexes.buffer,
				vertexes.getSize(),
				indexes.getSize(),
				materialRID
		};
		Soul::Render::RID meshRID = renderSystem->meshCreate(meshSpec);

		UIMesh sceneMesh = { 0 };
		strcpy(sceneMesh.name, "object");
		sceneMesh.rid = meshRID;
		sceneMesh.materialID = materialID;
		sceneMesh.scale = Soul::Vec3f(1.0f, 1.0f, 1.0f);
		sceneMesh.position = Soul::Vec3f(0.0f, 0.0f, 0.0f);
		sceneMesh.rotation = Soul::Vec4f(0.0f, 1.0f, 0.0f, 0.0f);
		sceneData->meshes.pushBack(sceneMesh);

		vertexes.cleanup();
		indexes.cleanup();

	}

	callbackData.vertexBuffer.position.cleanup();
	callbackData.vertexBuffer.normal.cleanup();
	callbackData.vertexBuffer.texCoord.cleanup();
	callbackData.indexCountBuffer.cleanup();
	callbackData.indexBuffer.cleanup();
	callbackData.materialIndexes.cleanup();
	callbackData.materialStartIndexes.cleanup();

}

bool ImportGLTFAssets(SceneData* sceneData, const char* gltfPath) {

	printf("GLTF Path : %s", gltfPath);
	
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, gltfPath);

	if (!warn.empty()) {
		SOUL_LOG_WARN("ImportGLTFAssets | %s", warn.c_str());
	}

	if (!err.empty()) {
		SOUL_LOG_ERROR("ImportGLTFAssets | %s", err.c_str());
		return false;
	}

	if (!ret) {
		SOUL_LOG_ERROR("ImportGLTFAssets | failed");
		return false;
	}

	Soul::Render::System& renderSystem = sceneData->renderSystem;

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
		
		Soul::Render::RID textureRID = renderSystem.textureCreate(texSpec, (unsigned char*)&image.image[0], image.component);

		UITexture uiTexture = { 0 };
		strcpy(uiTexture.name, texture.name.c_str());
		uiTexture.rid = textureRID;
		sceneData->textures.pushBack(uiTexture);
	
	}

	//Load Material
	for (int i = 0; i < model.materials.size(); i++) {
		UIMaterial uiMaterial = { 0 };
		const tinygltf::Material material = model.materials[i];

		if (material.values.count("baseColorFactor") != 0) {
			const tinygltf::ColorValue& colorValue = material.values.at("baseColorFactor").ColorFactor();
			uiMaterial.albedo = {
				(float)colorValue[0],
				(float)colorValue[1],
				(float)colorValue[2]
			};
		}

		if (material.values.count("baseColorTexture") != 0) {
			int texID = material.values.at("baseColorTexture").TextureIndex();
			uiMaterial.albedoTexID = texID + 1;
			uiMaterial.useAlbedoTex = true;
		}

		if (material.values.count("metallicFactor") != 0) {
			uiMaterial.metallic = material.values.at("metallicFactor").Factor();
		}

		if (material.values.count("roughnessFactor") != 0) {
			uiMaterial.roughness = material.values.at("roughnessFactor").Factor();
		}

		if (material.values.count("metallicRoughnessTexture") != 0) {
			int texID = material.values.at("metallicRoughnessTexture").TextureIndex();

			uiMaterial.metallicTexID = texID + 1;
			uiMaterial.metallicTextureChannel = Soul::Render::TexChannel_RED;
			uiMaterial.useMetallicTex = true;

			uiMaterial.roughnessTexID = texID + 1;
			uiMaterial.roughnessTextureChannel = Soul::Render::TexChannel_GREEN;
			uiMaterial.useRoughnessTex = true;
		}

		if (material.additionalValues.count("normalTexture")) {
			int texID = material.additionalValues.at("normalTexture").TextureIndex();

			uiMaterial.normalTexID = texID + 1;
			uiMaterial.useNormalTex = true;

		}

		if (material.additionalValues.count("occlusionTexture")) {
			int texID = material.additionalValues.at("occlusionTexture").TextureIndex();
			uiMaterial.aoTexID = texID + 1;
			uiMaterial.useAOTex = true;
		}

		SOUL_ASSERT(0, strlen(material.name.c_str()) <= 512, "Material name is too long | material.name = %s", material.name.c_str());
		strcpy(uiMaterial.name, material.name.c_str());

		uiMaterial.rid = renderSystem.materialCreate({
			sceneData->textures[uiMaterial.albedoTexID].rid,
			sceneData->textures[uiMaterial.normalTexID].rid,
			sceneData->textures[uiMaterial.metallicTexID].rid,
			sceneData->textures[uiMaterial.roughnessTexID].rid,
			sceneData->textures[uiMaterial.aoTexID].rid,

			uiMaterial.useAlbedoTex,
			uiMaterial.useNormalTex,
			uiMaterial.useMetallicTex,
			uiMaterial.useRoughnessTex,
			uiMaterial.useAOTex,

			Soul::Vec3f(0.0f, 0.0f, 0.0f),
			uiMaterial.metallic,
			uiMaterial.roughness,

			Soul::Render::TexChannel_BLUE,
			Soul::Render::TexChannel_GREEN,
			Soul::Render::TexChannel_RED
		});

		sceneData->materials.pushBack(uiMaterial);
	}

	

	// Load Mesh
	for (int i = 0; i < model.meshes.size(); i++) {

		Soul::Array<Soul::Render::Vertex> vertexes;
		vertexes.init(100000);
		Soul::Array<uint32> indexes;
		indexes.init(100000);
		
		const tinygltf::Mesh& mesh = model.meshes[i];
		SOUL_ASSERT(0, mesh.primitives.size() == 1, "Mesh with multiple number of primitive is not supported yet | mesh name = %s", mesh.name.c_str());
		
		const tinygltf::Primitive& primitive = mesh.primitives[0];

		const tinygltf::Accessor& positionAccessor = model.accessors[primitive.attributes.at("POSITION")];
		const tinygltf::Accessor& normalAccessor = model.accessors[primitive.attributes.at("NORMAL")];
		const tinygltf::Accessor& tangentAccessor = model.accessors[primitive.attributes.at("TANGENT")];
		const tinygltf::Accessor& texCoord0Accessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
		const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];

		SOUL_ASSERT(0, positionAccessor.count == normalAccessor.count, "");
		SOUL_ASSERT(0, tangentAccessor.count == texCoord0Accessor.count, "");
		SOUL_ASSERT(0, positionAccessor.count == tangentAccessor.count, "");

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

		SOUL_ASSERT(0, tangentAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Component type %d for tangent is not supported yet. | mesh name = %s.", tangentAccessor.componentType, mesh.name.c_str());
		SOUL_ASSERT(0, tangentAccessor.type == TINYGLTF_TYPE_VEC4, "Type %d for tangent is not supported yet. | mesh name = %s.", tangentAccessor.type, mesh.name.c_str());
		const tinygltf::BufferView& tangentBufferView = model.bufferViews[tangentAccessor.bufferView];
		int tangentOffset = tangentAccessor.byteOffset + tangentBufferView.byteOffset;
		int tangentByteStride = tangentAccessor.ByteStride(tangentBufferView);
		unsigned char* tangentBuffer = &model.buffers[tangentBufferView.buffer].data[tangentOffset];

		SOUL_ASSERT(0, texCoord0Accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Component type %d for texCoord0 is not supported yet. | mesh name = %s.", texCoord0Accessor.componentType, mesh.name.c_str());
		SOUL_ASSERT(0, texCoord0Accessor.type == TINYGLTF_TYPE_VEC2, "Type %d for texCoord0 is not supported yet. | mesh name = %s.", texCoord0Accessor.type, mesh.name.c_str());
		const tinygltf::BufferView& texCoord0BufferView = model.bufferViews[texCoord0Accessor.bufferView];
		int texCoord0Offset = texCoord0Accessor.byteOffset + texCoord0BufferView.byteOffset;
		int texCoord0ByteStride = texCoord0Accessor.ByteStride(texCoord0BufferView);
		unsigned char* texCoord0Buffer = &model.buffers[texCoord0BufferView.buffer].data[texCoord0Offset];

		for (int i = 0; i < positionAccessor.count; i++) {

			Soul::Vec3f position = *(Soul::Vec3f*)&positionBuffer[positionByteStride * i];
			Soul::Vec3f normal = *(Soul::Vec3f*)&normalBuffer[normalByteStride * i];
			Soul::Vec4f tangent = *(Soul::Vec4f*)&tangentBuffer[tangentByteStride * i];
			Soul::Vec2f texCoord0 = *(Soul::Vec2f*)&texCoord0Buffer[texCoord0ByteStride * i];

			vertexes.pushBack({
				position,
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
			indexes.pushBack(index);
		}

		Soul::Render::RID meshRID = renderSystem.meshCreate({
			Soul::mat4Identity(),
			vertexes.buffer,
			indexes.buffer,
			(uint32) vertexes.getSize(),
			(uint32) indexes.getSize(),
			sceneData->materials[primitive.material + 1].rid
		});

		UIMesh uiMesh;
		strcpy(uiMesh.name, mesh.name.c_str());
		uiMesh.rid = meshRID;
		uiMesh.scale = Soul::Vec3f(1.0f, 1.0f, 1.0f);
		uiMesh.position = Soul::Vec3f(0.0f, 0.0f, 0.0f);
		uiMesh.rotation = Soul::Vec4f(0.0f, 1.0f, 0.0f, 0.0f);
		uiMesh.materialID = primitive.material + 1;

		sceneData->meshes.pushBack(uiMesh);

		vertexes.cleanup();
		indexes.cleanup();

	}



	return true;

}