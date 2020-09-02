#include "renderer.h"

#include "core/dev_util.h"

#include "stb_image.h"

#include <imgui/imgui.h>
#include <GLFW/glfw3.h>

using namespace Soul;
namespace DeferredPipeline {

	void Renderer::init() {
		GPU::TextureDesc texDesc;
		texDesc.width = 1;
		texDesc.height = 1;
		texDesc.depth = 1;
		texDesc.type = GPU::TextureType::D2;
		texDesc.format = GPU::TextureFormat::RGBA8;
		texDesc.mipLevels = 1;
		texDesc.usageFlags = GPU::TEXTURE_USAGE_SAMPLED_BIT;
		texDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		texDesc.name = "Stub Texture";
		uint32 stubTextureData = 0;
		stubTexture = _gpuSystem->textureCreate(texDesc, (byte*)&stubTextureData, sizeof(stubTextureData));

		Vec2f quadVertices[] = {
			Vec2f(-1.0f, -1.0f),
			Vec2f(-1.0f, 1.0f),
			Vec2f(1.0f, -1.0f),
			Vec2f(1.0f, 1.0f)
		};
		GPU::BufferDesc quadBufferDesc;
		quadBufferDesc.typeSize = sizeof(Vec2f);
		quadBufferDesc.typeAlignment = alignof(Vec2f);
		quadBufferDesc.count = 4;
		quadBufferDesc.usageFlags = GPU::BUFFER_USAGE_VERTEX_BIT;
		quadBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		quadBuffer = _gpuSystem->bufferCreate(quadBufferDesc,
			[&quadVertices](int i, byte* data)
			{
				auto vertex = (Vec2f*)data;
				*vertex = quadVertices[i];
			});

		shadowMapGenRenderModule.init(_gpuSystem);
		gBufferGenRenderModule.init(_gpuSystem);
		finalGatherRenderModule.init(_gpuSystem);
		voxelizeRenderModule.init(_gpuSystem);
		voxelGIDebugRenderModule.init(_gpuSystem);
		voxelLightInjectRenderModule.init(_gpuSystem);
	}

	GPU::TextureNodeID Renderer::computeRenderGraph(GPU::RenderGraph* renderGraph)
	{
		Vec2ui32 sceneResolution = _scene.getViewport();

		GPU::RGTextureDesc renderTargetDesc;
		renderTargetDesc.width = sceneResolution.x;
		renderTargetDesc.height = sceneResolution.y;
		renderTargetDesc.depth = 1;
		renderTargetDesc.clear = true;
		renderTargetDesc.clearValue = {};
		renderTargetDesc.format = GPU::TextureFormat::RGBA8;
		renderTargetDesc.mipLevels = 1;
		renderTargetDesc.type = GPU::TextureType::D2;
		GPU::TextureNodeID finalRenderTarget = renderGraph->createTexture("FinalRender Target", renderTargetDesc);

		if (_scene.meshEntities.size() == 0) {
			return finalRenderTarget;
		}

		
		GPU::BufferDesc modelBufferDesc;
		modelBufferDesc.typeSize = sizeof(Mat4);
		modelBufferDesc.typeAlignment = alignof(Mat4);
		modelBufferDesc.count = _scene.meshEntities.size();
		modelBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		modelBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		GPU::BufferID modelBuffer = _gpuSystem->bufferCreate(modelBufferDesc,
			[&scene = this->_scene](int i, byte* data)
		{
			auto model = (Mat4*)data;
			*model = mat4Transpose(
				mat4Transform(scene.meshEntities[i].worldTransform));
		});
		_gpuSystem->bufferDestroy(modelBuffer);

		GPU::BufferDesc rotationBufferDesc;
		rotationBufferDesc.typeSize = sizeof(Mat4);
		rotationBufferDesc.typeAlignment = alignof(Mat4);
		rotationBufferDesc.count = _scene.meshEntities.size();
		rotationBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		rotationBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		GPU::BufferID rotationBuffer = _gpuSystem->bufferCreate(rotationBufferDesc,
			[&scene = this->_scene](int i, byte* data)
		{
			auto rotate = (Mat4*)data;
			*rotate = mat4Transpose(
				mat4Rotate(mat4Transform(scene.meshEntities[i].worldTransform)));
		});
		_gpuSystem->bufferDestroy(rotationBuffer);

		struct CameraUBO
		{
			Mat4 projection;
			Mat4 view;
			Mat4 projectionView;
			Mat4 invProjectionView;

			Vec3f position;
			float exposure;
		};
		CameraUBO cameraDataUBO;

		Camera& camera = _scene.camera;

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
		GPU::BufferID cameraBuffer = _gpuSystem->bufferCreate(cameraBufferDesc,
			[&cameraDataUBO](int i, byte* data) {
				auto cameraData = (CameraUBO*)data;
				*cameraData = cameraDataUBO;
			});
		_gpuSystem->bufferDestroy(cameraBuffer);

		struct DirLightUBO {
			Mat4 shadowMatrixes[4];
			Vec3f direction;
			float bias;
			Vec3f color;
			float preExposedIlluminance;
			float cascadeDepths[4];
		};

		DirLightUBO dirLightUBO = {};
		for (int i = 0; i < 4; i++)
		{
			dirLightUBO.shadowMatrixes[i] = mat4Transpose(_scene.dirLight.shadowMatrixes[i]);
		}
		dirLightUBO.direction = _scene.dirLight.direction;
		dirLightUBO.bias = _scene.dirLight.bias;
		dirLightUBO.color = _scene.dirLight.color;
		dirLightUBO.preExposedIlluminance = _scene.dirLight.illuminance;
		float cameraFar = camera.perspective.zFar;
		float cameraNear = camera.perspective.zNear;
		float cameraDepth = cameraFar - cameraNear;
		for (int j = 0; j < 3; j++) {
			dirLightUBO.cascadeDepths[j] = camera.perspective.zNear + (cameraDepth * _scene.dirLight.split[j]);
		}
		dirLightUBO.cascadeDepths[3] = cameraFar;

		GPU::BufferDesc lightBufferDesc;
		lightBufferDesc.typeSize = sizeof(DirLightUBO);
		lightBufferDesc.typeAlignment = alignof(DirLightUBO);
		lightBufferDesc.count = 1;
		lightBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		lightBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT | GPU::QUEUE_COMPUTE_BIT;
		GPU::BufferID lightBuffer = _gpuSystem->bufferCreate(lightBufferDesc,
			[&dirLightUBO](int i, byte* data) -> void
			{
				auto lightData = (DirLightUBO*)data;
				*lightData = dirLightUBO;
			});
		_gpuSystem->bufferDestroy(lightBuffer);

		GPU::BufferDesc shadowMatrixesBufferDesc;
		shadowMatrixesBufferDesc.typeSize = sizeof(Mat4);
		shadowMatrixesBufferDesc.typeAlignment = alignof(Mat4);
		shadowMatrixesBufferDesc.count = 4;
		shadowMatrixesBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		shadowMatrixesBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		GPU::BufferID shadowMatrixesBuffer = _gpuSystem->bufferCreate(shadowMatrixesBufferDesc,
			[&_scene = this->_scene](int i, byte* data) -> void {
			auto shadowMatrix = (Mat4*)data;
			*shadowMatrix = mat4Transpose(_scene.dirLight.shadowMatrixes[i]);
		});
		_gpuSystem->bufferDestroy(shadowMatrixesBuffer);

		Vec4f dummy = _scene.dirLight.shadowMatrixes[0] * Vec4f(100.0f, 1.0f, 1.0f, 1.0f);

		Array<GPU::TextureNodeID> sceneTextureNodeIDs;
		{
			sceneTextureNodeIDs.reserve(_scene.textures.size());
			for (const SceneTexture& sceneTexture : _scene.textures) {
				sceneTextureNodeIDs.add(renderGraph->importTexture("Scene Textures", sceneTexture.rid));
			}
		}

		struct VoxelGIDataUBO {
			Vec3f frustumCenter;
			int resolution;
			float frustumHalfSpan;
			float bias;
			float diffuseMultiplier;
			float specularMultiplier;
		};
		GPU::BufferDesc voxelGIDataBufferDesc;
		voxelGIDataBufferDesc.typeSize = sizeof(VoxelGIDataUBO);
		voxelGIDataBufferDesc.typeAlignment = alignof(VoxelGIDataUBO);
		voxelGIDataBufferDesc.count = 1;
		voxelGIDataBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		voxelGIDataBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT | GPU::QUEUE_COMPUTE_BIT;
		GPU::BufferID voxelGIDataBuffer = _gpuSystem->bufferCreate(voxelGIDataBufferDesc,
			[&_scene = this->_scene](int i, byte* data) -> void {
			VoxelGIDataUBO& voxelGIDataUBO = *(VoxelGIDataUBO*)data;
			voxelGIDataUBO.frustumCenter = _scene.voxelGIConfig.center;
			voxelGIDataUBO.resolution = _scene.voxelGIConfig.resolution;
			voxelGIDataUBO.bias = _scene.voxelGIConfig.bias;
			voxelGIDataUBO.frustumHalfSpan = _scene.voxelGIConfig.halfSpan;
			voxelGIDataUBO.diffuseMultiplier = _scene.voxelGIConfig.diffuseMultiplier;
			voxelGIDataUBO.specularMultiplier = _scene.voxelGIConfig.specularMultiplier;
		});
		_gpuSystem->bufferDestroy(voxelGIDataBuffer);

		struct VoxelGIMatrixesUBO {
			Mat4 projectionView[3];
			Mat4 invProjectionView[3];
		};
		GPU::BufferDesc voxelGIMatrixesBufferDesc;
		voxelGIMatrixesBufferDesc.typeSize = sizeof(VoxelGIMatrixesUBO);
		voxelGIMatrixesBufferDesc.typeAlignment = alignof(VoxelGIMatrixesUBO);
		voxelGIMatrixesBufferDesc.count = 1;
		voxelGIMatrixesBufferDesc.usageFlags = GPU::BUFFER_USAGE_UNIFORM_BIT;
		voxelGIMatrixesBufferDesc.queueFlags = GPU::QUEUE_GRAPHIC_BIT;
		GPU::BufferID voxelGIMatrixesBuffer = _gpuSystem->bufferCreate(voxelGIMatrixesBufferDesc,
			[&_scene = this->_scene](int i, byte* data) -> void {
			VoxelGIMatrixesUBO& voxelGIMatrixesUBO = *(VoxelGIMatrixesUBO*)data;

			float voxelFrustumHalfSpan = _scene.voxelGIConfig.halfSpan;
			Mat4 projection = mat4Ortho(
				-voxelFrustumHalfSpan, voxelFrustumHalfSpan,
				-voxelFrustumHalfSpan, voxelFrustumHalfSpan,
				-voxelFrustumHalfSpan, voxelFrustumHalfSpan);

			Vec3f voxelFrustumCenter = _scene.voxelGIConfig.center;
			Mat4 view[3];
			view[0] = mat4View(voxelFrustumCenter, voxelFrustumCenter + Vec3f(1.0f, 0.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f));
			view[1] = mat4View(voxelFrustumCenter, voxelFrustumCenter + Vec3f(0.0f, 1.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f));
			view[2] = mat4View(voxelFrustumCenter, voxelFrustumCenter + Vec3f(0.0f, 0.0f, 1.0f), Vec3f(0.0f, 1.0f, 0.0f));

			for (int i = 0; i < 3; i++) {
				voxelGIMatrixesUBO.projectionView[i] = mat4Transpose(projection * view[i]);
				voxelGIMatrixesUBO.invProjectionView[i] = mat4Transpose(mat4Inverse(projection * view[i]));
			}
		});
		_gpuSystem->bufferDestroy(voxelGIMatrixesBuffer);

		GPU::TextureNodeID stubTextureNodeID = renderGraph->importTexture("Stub Texture", stubTexture);

		GPU::BufferNodeID materialNodeID = renderGraph->importBuffer("Material Buffer", _scene.materialBuffer);
		GPU::BufferNodeID modelNodeID = renderGraph->importBuffer("Model Buffer", modelBuffer);
		GPU::BufferNodeID rotationNodeID = renderGraph->importBuffer("Rotate Buffer", rotationBuffer);
		GPU::BufferNodeID cameraNodeID = renderGraph->importBuffer("Camera buffer", cameraBuffer);
		GPU::BufferNodeID lightNodeID = renderGraph->importBuffer("Light buffer", lightBuffer);
		GPU::BufferNodeID shadowMatrixesNodeID = renderGraph->importBuffer("Shadow Matrixes buffer", shadowMatrixesBuffer);
		GPU::BufferNodeID voxelGIDataNodeID = renderGraph->importBuffer("Voxel GI Data buffer", voxelGIDataBuffer);
		GPU::BufferNodeID voxelGIMatrixesNodeID = renderGraph->importBuffer("Voxel GI Matrixes buffer", voxelGIMatrixesBuffer);

		Array<GPU::BufferNodeID> vertexBufferNodeIDs;
		{
			SOUL_PROFILE_ZONE_WITH_NAME("Create vertex buffer node ids");
			vertexBufferNodeIDs.reserve(_scene.meshes.size());
			for (const Mesh& mesh : _scene.meshes) {
				vertexBufferNodeIDs.add(renderGraph->importBuffer("Vertex buffer", mesh.vertexBufferID));
			}
		}

		Array<GPU::BufferNodeID> indexBufferNodeIDs;
		{
			SOUL_PROFILE_ZONE_WITH_NAME("Create index buffer node ids");
			indexBufferNodeIDs.reserve(_scene.meshes.size());
			for (const Mesh& mesh : _scene.meshes) {
				indexBufferNodeIDs.add(renderGraph->importBuffer("Index buffer", mesh.indexBufferID));
			}
		}

		GPU::RGTextureDesc shadowMapTexDesc;
		shadowMapTexDesc.width = DirectionalLight::SHADOW_MAP_RESOLUTION;
		shadowMapTexDesc.height = DirectionalLight::SHADOW_MAP_RESOLUTION;
		shadowMapTexDesc.depth = 1;
		shadowMapTexDesc.clear = true;
		shadowMapTexDesc.clearValue.depthStencil = { 1.0f, 0 };
		shadowMapTexDesc.format = GPU::TextureFormat::DEPTH32F;
		shadowMapTexDesc.mipLevels = 1;
		shadowMapTexDesc.type = GPU::TextureType::D2;
		GPU::TextureNodeID shadowMapNodeID = renderGraph->createTexture("Shadow Map", shadowMapTexDesc);

		ShadowMapGenRenderModule::Parameter shadowMapGenData;
		shadowMapGenData.shadowMatrixesBuffer = shadowMatrixesNodeID;
		shadowMapGenData.modelBuffer = modelNodeID;
		shadowMapGenData.depthTarget = shadowMapNodeID;
		shadowMapGenData.vertexBuffers = vertexBufferNodeIDs;
		shadowMapGenData.indexBuffers = indexBufferNodeIDs;
		shadowMapGenData = shadowMapGenRenderModule.addPass(_gpuSystem, renderGraph, shadowMapGenData, _scene);

		GBufferGenRenderModule::Parameter gBufferGenParam;
		gBufferGenParam.vertexBuffers = vertexBufferNodeIDs;
		gBufferGenParam.indexBuffers = indexBufferNodeIDs;
		gBufferGenParam.sceneTextures = sceneTextureNodeIDs;
		gBufferGenParam.camera = cameraNodeID;
		gBufferGenParam.light = lightNodeID;
		gBufferGenParam.material = materialNodeID;
		gBufferGenParam.model = modelNodeID;
		
		for (GPU::TextureNodeID& nodeID : gBufferGenParam.renderTargets) {
			nodeID = renderGraph->createTexture("Render Target", renderTargetDesc);
		}
		GPU::RGTextureDesc sceneDepthTexDesc;
		sceneDepthTexDesc.width = sceneResolution.x;
		sceneDepthTexDesc.height = sceneResolution.y;
		sceneDepthTexDesc.depth = 1;
		sceneDepthTexDesc.clear = true;
		sceneDepthTexDesc.clearValue.depthStencil = { 1.0f, 0 };
		sceneDepthTexDesc.format = GPU::TextureFormat::DEPTH32F;
		sceneDepthTexDesc.mipLevels = 1;
		sceneDepthTexDesc.type = GPU::TextureType::D2;
		gBufferGenParam.depthTarget = renderGraph->createTexture("Depth target", sceneDepthTexDesc);
		gBufferGenParam.shadowMap = shadowMapGenData.depthTarget;
		gBufferGenParam.stubTexture = stubTextureNodeID;
		gBufferGenRenderModule.addPass(_gpuSystem, renderGraph, gBufferGenParam, _scene);

		VoxelizeRenderModule::Parameter voxelizeParam;
		voxelizeParam.stubTexture = stubTextureNodeID;
		voxelizeParam.vertexBuffers = vertexBufferNodeIDs;
		voxelizeParam.indexBuffers = indexBufferNodeIDs;
		voxelizeParam.model = modelNodeID;
		voxelizeParam.rotation = rotationNodeID;
		voxelizeParam.voxelGIData = voxelGIDataNodeID;
		GPU::RGTextureDesc voxelTargetTexDesc;
		voxelTargetTexDesc.width = _scene.voxelGIConfig.resolution;
		voxelTargetTexDesc.height = _scene.voxelGIConfig.resolution;
		voxelTargetTexDesc.depth = _scene.voxelGIConfig.resolution;
		voxelTargetTexDesc.clear = true;
		voxelTargetTexDesc.clearValue.color.uint32 = {};
		voxelTargetTexDesc.format = GPU::TextureFormat::R32UI;
		voxelTargetTexDesc.mipLevels = 1;
		voxelTargetTexDesc.type = GPU::TextureType::D3;
		voxelizeParam.voxelAlbedo = renderGraph->createTexture("Voxel Albedo Target", voxelTargetTexDesc);
		voxelizeParam.voxelEmissive = renderGraph->createTexture("Voxel Emissive target", voxelTargetTexDesc);
		voxelizeParam.voxelNormal = renderGraph->createTexture("Voxel Normal target", voxelTargetTexDesc);
		voxelizeParam.material = materialNodeID;
		voxelizeParam.materialTextures = sceneTextureNodeIDs;
		voxelizeParam.voxelizeMatrixes = voxelGIMatrixesNodeID;
		voxelizeParam = voxelizeRenderModule.addPass(_gpuSystem, renderGraph, voxelizeParam, _scene);

		VoxelLightInjectRenderModule::Parameter voxelInjectParam;
		voxelInjectParam.voxelAlbedo = voxelizeParam.voxelAlbedo;
		voxelInjectParam.voxelNormal = voxelizeParam.voxelNormal;
		voxelInjectParam.voxelEmissive = voxelizeParam.voxelEmissive;
		GPU::RGTextureDesc voxelLightTexDesc = voxelTargetTexDesc;
		voxelLightTexDesc.format = GPU::TextureFormat::RGBA16F;
		voxelLightTexDesc.mipLevels = int(log2f(_scene.voxelGIConfig.resolution));
		voxelInjectParam.voxelLight = renderGraph->createTexture("Voxel light Target", voxelLightTexDesc);
		voxelInjectParam.voxelGIData = voxelGIDataNodeID;
		voxelInjectParam.lightData = lightNodeID;
		voxelInjectParam = voxelLightInjectRenderModule.addPass(_gpuSystem, renderGraph, voxelInjectParam, _scene);

		/* VoxelGIDebugRenderModule::Parameter voxelGIDebugParam;
		voxelGIDebugParam.renderTarget = renderGraph->createTexture("Voxel GI color target", renderTargetDesc);
		voxelGIDebugParam.depthTarget = renderGraph->createTexture("Voxel GI depth target", sceneDepthTexDesc);
		voxelGIDebugParam.voxelGIData = voxelGIDataNodeID;
		voxelGIDebugParam.cameraData = cameraNodeID;
		voxelGIDebugParam.voxelAlbedo = voxelizeParam.voxelAlbedo;
		voxelGIDebugParam.voxelEmissive = voxelizeParam.voxelEmissive;
		voxelGIDebugParam.voxelNormal = voxelizeParam.voxelNormal;
		voxelGIDebugParam.voxelLight = voxelInjectParam.voxelLight;
		voxelGIDebugParam = voxelGIDebugRenderModule.addPass(_gpuSystem, renderGraphh, voxelGIDebugParam, scene); */

		GPU::BufferNodeID quadBufferNodeID = renderGraph->importBuffer("Quad Buffer", quadBuffer);
		FinalGatherRenderModule::Parameter finalGatherParameter;
		for (int i = 0; i < 4; i++) {
			finalGatherParameter.renderMap[i] = gBufferGenParam.renderTargets[i];
		}
		finalGatherParameter.renderTarget = finalRenderTarget;
		finalGatherParameter.vertexBuffer = quadBufferNodeID;
		finalGatherParameter = finalGatherRenderModule.addPass(_gpuSystem, renderGraph, finalGatherParameter, sceneResolution);

		return finalGatherParameter.renderTarget;
	}

}
