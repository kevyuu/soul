#pragma once

#include "core/type.h"
#include "core/packed_pool.h"
#include "core/pool.h"
#include "gpu/data.h"
#include "gpu/render_graph.h"

namespace Soul{
	class RenderGraph;
}

namespace KyuRen {
	using namespace Soul;

	enum class ResourceType : uint8 {
		MESH,
		SUNLIGHT,
		COUNT
	};
	
	struct ResourceID {
		
		uint64 id;

		static constexpr uint32 TYPE_BITS_SHIFT = sizeof(id) - sizeof(ResourceType);
		static constexpr uint64 TYPE_BITS_MASK = ~((1u << TYPE_BITS_SHIFT) - 1u);

		ResourceID(uint64 id) {
			this->id = id;
		}

		ResourceID(uint8 type, PoolID internalIndex) {
			id = type << TYPE_BITS_SHIFT | internalIndex;
		}

		uint8 type() {
			return id >> TYPE_BITS_SHIFT;
		}

		PoolID index() {
			return id & ~(TYPE_BITS_MASK);
		}
	};

	struct Mesh {
		GPU::BufferID posVertexBufferID;
		GPU::BufferID norVertexBufferID;
		uint32 vertexCount;
		GPU::BufferID indexBufferID;
		uint32 indexCount;
	};

	struct SpotLight {
		Vec3f color;
		float cutoffDistance;
		float distance;
		float specularFactor;

		float constantCoefficient;
		float contachShadowBias;
		float contactShadowDistance;
		float contactShadowThickness;
		float energy;
		
		// CurveMap falloffCurve;
		// FalloffType falloffType;

		float linearAttenuation;
		float linearCoefficient;
		float quadraticAttenuation;
		float quadraticCoefficient;
		float shadowBufferBias;
		float shadowBufferClipStart;
		uint8 shadowBufferSamples;
		uint16 shadowBufferSize;
		Vec3f shadowColor;
		float shadowSoftSize;

		bool useContactShadow;
		bool useShadow;
		bool useSquare;
	};

	struct SunLight {
		Vec3f color;
		float cutoffDistance;
		float distance;
		float specularFactor;

		float angle;
		float contactShadowBias;
		float contactShadowDistance;
		float contactShadowThickness;
		float energy;
		float shadowBufferBias;
		float shadowBufferClipStart;
		uint8 shadowBufferSamples;
		uint16 shadowBufferSize;
		uint8 shadowCascadeCount;
		float shadowCascadeExponent;
		float shadowCascadeFade;
		float shadowCascadeMaxDistance;
		Vec3f shadowColor;
		float shadowSoftSize;
		bool useContactShadow;
		bool useShadow;
	};

	struct Material {

	};

	struct MeshEntity {
		Mat4 worldMatrix;
		PoolID meshID;
	};

	enum CameraType {
		ORTHO,
		PERSPECTIVE
	};

	struct Camera {
		CameraType type;
		Vec3f origin;
		Vec3f up;
		Vec3f target;

		Mat4 viewMatrix;
		Mat4 projectionMatrix;

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

		Vec2ui32 viewDim;
	};

	struct SunLightEntity {
		Mat4 worldMatrix;
		PoolID sunLightID;

		float split[3] = { 0.1f, 0.2f, 0.7f };
		Mat4 shadowMatrixes[4];

		static constexpr uint32 SHADOW_MAP_RESOLUTION = 2048 * 2;

		void updateShadowMatrixes(const Camera& camera)
		{

			float zNear = camera.perspective.zNear;
			float zFar = camera.perspective.zFar;
			float zDepth = 200 - zNear;
			float fov = camera.perspective.fov;
			float aspectRatio = camera.perspective.aspectRatio;
			Vec3f upVec = Vec3f(0.0f, 1.0f, 0.0f);

			Soul::Vec3f direction = (worldMatrix * Vec3f(0, 0, 1)) - (worldMatrix * Vec3f(0, 0, 0));
			direction *= -1;
			direction = Soul::unit(direction);

			if (abs(dot(upVec, direction)) == 1.0f)
			{
				upVec = Vec3f(1.0f, 0.0f, 0.0f);
			}
			Mat4 lightRot = mat4View(Vec3f(0, 0, 0), direction, upVec);

			Mat4 viewMat = camera.viewMatrix;


			float splitOffset[5] = { 0, split[0], split[1], split[2], 1 };
			float splitNDCWidth = 1.0f;
			for (int i = 0; i < 4; i++)
			{
				Vec3f frustumCorners[8] = {
					   Vec3f(-1.0f, -1.0f, -1), Vec3f(1.0f, -1.0f, -1), Vec3f(1.0f, 1.0f, -1), Vec3f(-1.0f, 1.0f, -1),
					   Vec3f(-1.0f, -1.0f, 1), Vec3f(1.0f, -1.0f, 1), Vec3f(1.0f, 1.0f, 1), Vec3f(-1.0f, 1.0f, 1)
				};

				Mat4 projectionMat = mat4Perspective(fov, aspectRatio, zNear + splitOffset[i] * zDepth, zNear + splitOffset[i + 1] * zDepth);
				Mat4 projectionViewMat = projectionMat * viewMat;
				Mat4 invProjectionViewMat = mat4Inverse(projectionViewMat);

				Vec3f worldFrustumCenter = Vec3f(0, 0, 0);

				for (int k = 0; k < 8; k++) {
					Vec4f frustumCorner = invProjectionViewMat * Vec4f(frustumCorners[k], 1.0f);
					frustumCorners[k] = frustumCorner.xyz() / frustumCorner.w;
					worldFrustumCenter += frustumCorners[k];
				}
				worldFrustumCenter *= (1.0f / 8.0f);

				float cascadeDepth = (splitOffset[i + 1] - splitOffset[i]) * zDepth;
				float cascadeFarDistance = zNear + splitOffset[i + 1] * zDepth;
				float cascadeFarWidth = tan(camera.perspective.fov / 2) * 2 * cascadeFarDistance;
				float cascadeFarHeight = cascadeFarWidth / camera.perspective.aspectRatio;

				float radius = sqrt(cascadeFarWidth * cascadeFarWidth + cascadeDepth * cascadeDepth + cascadeFarHeight * cascadeFarHeight);

				float texelPerUnit = SHADOW_MAP_RESOLUTION / (radius * 4.0f);
				Mat4 texelScaleLightRot = mat4Scale(Soul::Vec3f(texelPerUnit, texelPerUnit, texelPerUnit)) * lightRot;

				Vec3f lightTexelFrustumCenter = texelScaleLightRot * worldFrustumCenter;
				lightTexelFrustumCenter.x = (float)floor(lightTexelFrustumCenter.x);
				lightTexelFrustumCenter.y = (float)floor(lightTexelFrustumCenter.y);
				worldFrustumCenter = mat4Inverse(texelScaleLightRot) * lightTexelFrustumCenter;

				int xSplit = i % 2;
				int ySplit = i / 2;

				float bottomSplitNDC = -1 + (ySplit * splitNDCWidth);
				float leftSplitNDC = -1 + (xSplit * splitNDCWidth);

				Mat4 atlasMatrix;
				atlasMatrix.elem[0][0] = splitNDCWidth / 2;
				atlasMatrix.elem[0][3] = leftSplitNDC + (splitNDCWidth * 0.5f);
				atlasMatrix.elem[1][1] = splitNDCWidth / 2;
				atlasMatrix.elem[1][3] = bottomSplitNDC + (splitNDCWidth * 0.5f);
				atlasMatrix.elem[2][2] = 1;
				atlasMatrix.elem[3][3] = 1;

				/*Vec3f sceneBoundCorners[8] = {
					db.sceneBound.min,
					Vec3f(db.sceneBound.min.x, db.sceneBound.min.y, db.sceneBound.max.z),
					Vec3f(db.sceneBound.min.x, db.sceneBound.max.y, db.sceneBound.min.z),
					Vec3f(db.sceneBound.min.x, db.sceneBound.max.y, db.sceneBound.max.z),
					Vec3f(db.sceneBound.max.x, db.sceneBound.min.y, db.sceneBound.min.z),
					Vec3f(db.sceneBound.max.x, db.sceneBound.min.y, db.sceneBound.max.z),
					Vec3f(db.sceneBound.max.x, db.sceneBound.max.y, db.sceneBound.min.z),
					db.sceneBound.max
				};

				float shadowMapFar = dot(direction, (sceneBoundCorners[0] - worldFrustumCenter));
				float shadowMapNear = shadowMapFar;

				for (int j = 1; j < 8; j++) {
					float cornerDist = dot(direction, sceneBoundCorners[j] - worldFrustumCenter);
					if (cornerDist > shadowMapFar) shadowMapFar = cornerDist;
					if (cornerDist < shadowMapNear) shadowMapNear = cornerDist;
				}*/
				float shadowMapFar = 500;
				float shadowMapNear = -500;

				shadowMatrixes[i] = atlasMatrix * mat4Ortho(-radius, radius, -radius, radius, shadowMapNear, shadowMapFar) *
					mat4View(worldFrustumCenter, worldFrustumCenter + direction, upVec);
			}
		}
	};

	struct Scene {

		Camera camera;
		
		Array<MeshEntity> meshEntities;
		Array<SunLightEntity> sunLightEntities;
		
		PackedPool<Mesh> meshes;
		PackedPool<SunLight> sunLights;

	};

	struct RenderJobParam {

		enum class Type : uint8 {
			GPUBuffer,
			GPUTexture1D,
			GPUTexture2D,
			GPUTexture3D,
			GPUBufferArray
		};

		Type type;

		const char* name;
		const char* desc;

	};

	struct RenderJobArg {

		RenderJobParam::Type type;
		union {
			Soul::GPU::TextureNodeID textureNodeID;
			Soul::GPU::BufferNodeID bufferNodeID;
			Soul::Array<Soul::GPU::BufferNodeID>* bufferArrays;
		} val;

	};

	class RenderJobInputs;
	class RenderJobOutputs;

	class RenderJob {
	private:

		struct ParamList {

			Soul::UInt64HashMap<uint32> nameToIndex;
			Soul::Array<RenderJobParam> params;

			inline const RenderJobParam& operator[](int idx) const { return params[idx]; }
			inline RenderJobParam& operator[](int idx) { return params[idx]; }

			static uint64 hashName(const char* name);

			inline const RenderJobParam& operator[](const char* name) const { return params[nameToIndex[hashName(name)]]; }
			inline RenderJobParam& operator[](const char* name) { return params[nameToIndex[hashName(name)]]; }

			inline bool isExist(const char* name) const { return nameToIndex.isExist(hashName(name)); }
			inline uint32 getIndex(const char* name) const { return nameToIndex[hashName(name)]; }

			inline uint32 add(const char* name, const RenderJobParam& param) {
				uint32 idx = params.add(param);
				nameToIndex.add(hashName(name), idx);
				return idx;
			}

		};

		ParamList _inputs;
		ParamList _outputs;
		ParamList _inputOutputs;

	protected:

		inline void _addInput(const RenderJobParam& param) { _inputs.add(param.name, param); }
		inline void _addOutput(const RenderJobParam& param) { _outputs.add(param.name, param); }
		inline void _addInputOutput(const RenderJobParam& param) { _inputOutputs.add(param.name, param); }

	public:
		virtual void init(const Scene*, Soul::GPU::System*) = 0;
		virtual void execute(Soul::GPU::RenderGraph* renderGraph, const RenderJobInputs& inputs, RenderJobOutputs& outputs) = 0;
		virtual void cleanup() { /* do nothing */ };

		uint32 getInputCount() const { return _inputs.params.size(); }
		uint32 getOutputCount() const { return _outputs.params.size(); }
		uint32 getInputOutputCount() const { return _inputOutputs.params.size(); }

		int32 getInputIndex(const char* name) const {
			if (_inputs.isExist(name)) {
				return _inputs.getIndex(name);
			}
			if (_inputOutputs.isExist(name)) {
				return getInputCount() + _inputOutputs.getIndex(name);
			}
			return -1;
		}

		int32 getOutputIndex(const char* name) const {
			if (_outputs.isExist(name)) {
				return _outputs.getIndex(name);
			}
			if (_inputOutputs.isExist(name)) {
				return getOutputCount() + _inputOutputs.getIndex(name);
			}
			return -1;
		}

		friend class RenderJobInputs;
		friend class RenderJobOutputs;
		friend class RenderJobPipeline;
	};

	using RenderJobID = Soul::ID<RenderJob, PackedID>;
	static constexpr RenderJobID RENDER_JOB_ID_NULL = RenderJobID(SOUL_UTYPE_MAX(PackedID));	

	

	class RenderJobInputs {
	
	private:
		const RenderJob* _renderJob;
		Soul::Array<RenderJobArg> _renderJobArgs;

	public:

		RenderJobInputs(const RenderJob* renderJob, Soul::Memory::Allocator* allocator) : _renderJob(renderJob), _renderJobArgs(allocator) {
			_renderJobArgs.resize(renderJob->getInputCount() + renderJob->getInputOutputCount());
		}
		
		inline const RenderJobArg& operator[](int idx) const {
			return _renderJobArgs[idx];
		}

		inline RenderJobArg& operator[](int idx) {
			return _renderJobArgs[idx];
		}

		inline const RenderJobArg& operator[](const char* name) const {
			return _renderJobArgs[_renderJob->getInputIndex(name)];
		}

		inline RenderJobArg& operator[](const char* name) {
			return _renderJobArgs[_renderJob->getInputIndex(name)];
		}

	};

	struct RenderJobOutputs {
	private:
		const RenderJob* _renderJob;
		Soul::Array<RenderJobArg> _renderJobArgs;

	public:

		RenderJobOutputs(const RenderJob* renderJob, Soul::Memory::Allocator* allocator) : _renderJob(renderJob), _renderJobArgs(allocator) {
			_renderJobArgs.resize(renderJob->getOutputCount() + renderJob->getInputOutputCount());
		}

		inline const RenderJobArg& operator[](int idx) const {
			return _renderJobArgs[idx];
		}

		inline RenderJobArg& operator[](int idx) {
			return _renderJobArgs[idx];
		}

		inline const RenderJobArg& operator[](const char* name) const {
			return _renderJobArgs[_renderJob->getOutputIndex(name)];
		}

		inline RenderJobArg& operator[](const char* name) {
			return _renderJobArgs[_renderJob->getOutputIndex(name)];
		}
	};

	

	class RenderPipeline {
	private:

		struct Socket {
			RenderJobID jobID;
			uint32 paramIndex;

			Socket() : jobID(RENDER_JOB_ID_NULL) {}

			Socket(RenderJobID jobID, uint32 paramIndex) : jobID(jobID), paramIndex(paramIndex) {}

			bool operator==(const Socket& rhs)
			{
				return jobID == rhs.jobID && paramIndex == rhs.paramIndex;
			}
		};

		struct OutputEdge {
			uint32 sourceParamIndex;
			Socket targetSocket;
		};

		struct RenderJobInstance {
			RenderJob* renderJob;
			Soul::Array<Socket> inputs;
			Soul::Array<OutputEdge> outputs;
		};

		Socket _output;
		Soul::PackedPool<RenderJobInstance> _jobs;
		Soul::Array<RenderJobID> _executionOrder;
		Soul::GPU::System* _gpuSystem;

		void _removeSourceLink(RenderJobID sourceJob, uint32 sourceParamIndex, RenderJobID targetJob, uint32 targetParamIndex);
		void _removeTargetLink(RenderJobID sourceJob, uint32 sourceParamIndex, RenderJobID targetJob, uint32 targetParamIndex);
		void _removeEdge(RenderJobID sourceJob, uint32 sourceParamIndex, RenderJobID targetjob, uint32 targetParamIndex);

	public:

		RenderPipeline(Soul::GPU::System* gpuSystem) : _gpuSystem(gpuSystem) {}

		RenderJobID addJob(RenderJob* renderJob);
		void removeJob(RenderJobID jobID);
		void removeEdge(RenderJobID sourceJob, const char* sourceParam, RenderJobID targetJob, const char* targetParam);
		void connect(RenderJobID sourceJob, const char* sourceParam, RenderJobID targetJob, const char* targetParam);
		void setOutput(RenderJobID job, const char* param);
		void compile();
		void execute(char* pixels);
	};

}