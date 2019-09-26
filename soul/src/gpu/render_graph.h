#pragma once
#include "gpu/data.h"
#include <limits.h>

namespace Soul { namespace GPU {

	using RenderGraphTextureID = uint16;
	using RenderGraphBufferID = uint16;

	using TextureNodeID = uint16;
	using BufferNodeID = uint16;

	using PassNodeID = uint16;

	enum class Topology : uint8 {
		POINT_LIST,
		LINE_LIST,
		LINE_STRIP,
		TRIANGLE_LIST,
		TRIANGLE_STRIP,
		TRIANGLE_FAN,
		COUNT
	};

	enum class PolygonMode : uint8 {
		FILL,
		LINE,
		POINT,
		COUNT
	};

	enum class CullMode : uint8 {
		NONE,
		FRONT,
		BACK,
		FRONT_AND_BACK,
		COUNT
	};

	enum class FrontFace : uint8 {
		CLOCKWISE,
		COUNTER_CLOCKWISE,
		COUNT
	};

	enum class CompareOp : uint8 {
		NEVER,
		LESS,
		EQUAL,
		LESS_OR_EQUAL,
		GREATER,
		NOT_EQUAL,
		GREATER_OR_EQUAL,
		ALWAYS,
		COUNT
	};

	enum class BlendFactor : uint8 {
		ZERO,
		ONE,
		SRC_COLOR,
		ONE_MINUS_SRC_COLOR,
		DST_COLOR,
		ONE_MINUS_DST_COLOR,
		SRC_ALPHA,
		ONE_MINUS_SRC_ALPHA,
		DST_ALPHA,
		CONSTANT_COLOR,
		ONE_MINUS_CONSTANT_COLOR,
		CONSTANT_ALPHA,
		ONE_MINUS_CONSTANT_ALPHA,
		SRC_ALPHA_SATURATE,
		SRC1_COLOR,
		ONE_MINUS_SRC1_COLOR,
		SRC1_ALPHA,
		ONE_MINUS_SRC1_ALPHA,
		COUNT
	};

	enum class BlendOp : uint8 {
		ADD,
		SUBTRACT,
		REVERSE_SUBTRACT,
		MIN,
		MAX,
		COUNT
	};

	struct ClearValue {
		Vec3f color;
		Vec2f depthStencil;
	};

	namespace Command {

		static constexpr uint32 MAX_ARGUMENT_PER_SHADER = 4;

		struct Command {};

		struct DrawIndex : Command {
			GPU::ShaderArgumentID shaderArguments[MAX_ARGUMENT_PER_SHADER];
			GPU::BufferID vertexBufferID;
			GPU::BufferID indexBufferID;
		};

		template<typename T>
		struct UpdateTypeBuffer : Command {
			GPU::BufferID bufferID;
			T* data;
			uint16 index;
		};

	};
	
	struct RenderGraphTextureDesc {
		uint32 width;
		uint32 height;
		TextureFormat format;
		bool clear;
		ClearValue clearValue;
	};

	struct RenderGraphTexture {
		const char* name;
		RenderGraphTextureDesc desc;
	};

	struct RenderGraphBufferDesc {
		uint32 count;
	};

	struct RenderGraphBuffer {
		const char* name;
		RenderGraphBufferDesc desc;
	};

	struct CommandBucket {
		uint64* keys;
		Array<Command::Command*> commands;

		void reserve(uint32 capacity);

		template <typename Command>
		Command* put(uint32 index, uint64 key);
	};

	struct GraphicPipelineDesc {

		struct ShaderDesc {
			char* vertexPath = nullptr;
			char* geometryPath = nullptr;
			char* fragmentPath = nullptr;
		} shaderDesc;

		struct InputLayoutDesc {
			Topology topology = Topology::TRIANGLE_LIST;
		} inputLayoutDesc;

		struct RasterDesc {
			float lineWidth;
			PolygonMode polygonMode = PolygonMode::FILL;
			CullMode cullMode = CullMode::NONE;
			FrontFace frontFace = FrontFace::CLOCKWISE;
		} rasterDesc;

		struct DepthStencilDesc {
			bool depthTestEnable = false;
			bool depthWriteEnable = false;
			CompareOp depthCompareOp = CompareOp::NEVER;
			TextureNodeID attachment = 0;
		} depthStencilDesc;

		struct ColorBlendDesc {
			bool blendEnable = false;
			BlendFactor srcColorBlendFactor = BlendFactor::ZERO;
			BlendFactor dstColorBlendFactor = BlendFactor::ZERO;
			BlendOp blendOp = BlendOp::ADD;
			BlendFactor srcAlphaBlendFactor = BlendFactor::ZERO;
			BlendFactor dstAlphaBlendFactor = BlendFactor::ZERO;

			TextureNodeID attachments[8] = {0};
		} colorDesc;

	};

	enum class PassType : uint8 {
		GRAPHIC,
		COMPUTE,
		TRANSFER,
		COUNT
	};

	enum class TextureReadUsage : uint8 {
		READ_MODIFY,
		TRANSFER_SRC,
		DEPTH_STENCIL_SAMPLED,
		COLOR_SAMPLED,
		INPUT_ATTACHMENT,
		STORAGE,
		COUNT
	};

	enum class TextureWriteUsage : uint8 {
		TRANSFER_DST,
		STORAGE,
		COLOR_ATTACHMENT,
		DEPTH_ATTACHMENT,
		COUNT
	};

	enum class BufferReadUsage : uint8 {
		READ_MODIFY,
		TRANSFER_SRC,
		VERTEX,
		INDEX,
		INDIRECT_BUFFER,
		UNIFORM,
		SAMPLED,
		STORAGE,
		COUNT
	};

	enum class BufferWriteUsage : uint8 {
		TRANSFER_DST,
		STORAGE,
		COUNT
	};

	struct BufferReadEntry {
		BufferNodeID nodeID;
		BufferReadUsage usage;
	};

	struct BufferWriteEntry {
		BufferNodeID nodeID;
		BufferWriteUsage usage;
	};

	struct TextureReadEntry {
		TextureNodeID nodeID;
		TextureReadUsage usage;
	};

	struct TextureWriteEntry {
		TextureNodeID nodeID;
		TextureWriteUsage usage;
	};

	struct PassNode {
		const char* name;
		PassType type;

		Array<BufferReadEntry> bufferReads;
		Array<BufferWriteUsage> bufferWrites;

		Array<TextureReadEntry> textureReads;
		Array<TextureWriteEntry> textureWrites;
		
	};

	template<typename Data, typename Execute>
	struct GraphicNode : PassNode {
		Data data;
		Execute execute;
		GraphicPipelineDesc pipelineDesc;
		CommandBucket commandBucket;
	};

	template<typename Data, typename Execute>
	struct TransferNode : PassNode {
		Data data;
		Execute execute;
		CommandBucket commandBucket;
	};

	template<typename Data, typename Execute>
	struct ComputeNode : PassNode {
		Data data;
		Execute execute;
		CommandBucket commandBucket;
	};

	struct TextureNode {
		RenderGraphTextureID resourceID;
		PassNodeID writer;
		Array<PassNodeID> readers;
	};

	struct BufferNode {
		RenderGraphBufferID resourceID;
		PassNodeID writer;
		Array<PassNodeID> readers;
	};

	struct RenderGraph {

		static constexpr TextureNodeID SWAPCHAIN_TEXTURE = USHRT_MAX;

		template<typename T, typename Setup, typename Execute>
		GraphicNode<T, Execute>& addGraphicPass(const char* name, Setup setup, Execute execute);

		template<typename T, typename Setup, typename Execute>
		TransferNode<T, Execute>& addTransferPass(const char* name, Setup setup, Execute execute);

		template<typename T, typename Setup, typename Execute>
		ComputeNode<T, Execute>& addComputePass(const char* name, Setup setup, Execute execute);

		TextureNodeID importTexture(const char* name, TextureID textureID);
		TextureNodeID createTexture(const char* name, const RenderGraphTextureDesc& desc, TextureWriteUsage usage);
		TextureNodeID writeTexture(TextureNodeID texture, TextureWriteUsage usage);
		TextureNodeID readTexture(TextureNodeID texture, TextureReadUsage usage);

		BufferNodeID importBuffer(const char* name, BufferID bufferID);
		template<typename T>
		BufferNodeID createBuffer(const RenderGraphBufferDesc& desc, BufferWriteUsage usage);
		BufferNodeID writeBuffer(BufferNodeID buffer, BufferWriteUsage usage);
		BufferNodeID readBuffer(BufferNodeID buffer, BufferReadUsage usage);

		BufferID getBuffer(BufferNodeID resource);
		ShaderArgumentID getShaderArgument(GPU::BufferID bufferID, uint32 offset);

		PassNodeID currentPassNodeID;
		Array<PassNode*> passNodes;
		
		Array<BufferNode> bufferNodes;
		Array<TextureNode> textureNodes;

		Array<RenderGraphTexture> textures;
		Array<RenderGraphBuffer> buffers;

	};
}}