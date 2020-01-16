#pragma once
#include "core/util.h"
#include "gpu/data.h"

namespace Soul { namespace GPU {

	struct RenderGraph;
	struct RenderGraphRegistry;

	//ID
	struct PassNode;
	using PassNodeID = ID<PassNode, uint16>;
	static constexpr PassNodeID PASS_NODE_ID_NULL = PassNodeID(SOUL_UTYPE_MAX(PassNodeID::id));

	struct _TextureNode;
	using TextureNodeID = ID<_TextureNode, uint16>;
	static constexpr TextureNodeID TEXTURE_NODE_ID_NULL = TextureNodeID(SOUL_UTYPE_MAX(TextureNodeID::id));

	struct _BufferNode;
	using BufferNodeID = ID<_BufferNode, uint16>;
	static constexpr BufferNodeID  BUFFER_NODE_ID_NULL = BufferNodeID(SOUL_UTYPE_MAX(BufferNodeID));

	struct _RGResourceID {
		uint32 index = SOUL_UTYPE_MAX(uint32);

		static constexpr uint8 EXTERNAL_BIT_POSITION = 31;

		static _RGResourceID InternalID(uint32 index) {return { index }; }
		static _RGResourceID ExternalID(uint32 index) {return { index | 1 << EXTERNAL_BIT_POSITION }; }

		bool isExternal() const {
			return index & (1u << EXTERNAL_BIT_POSITION);
		}

		uint32 getIndex() const {
			return index & ~(1u << EXTERNAL_BIT_POSITION);
		}

	};
	static constexpr _RGResourceID _RG_RESOURCE_ID_NULL = { SOUL_UTYPE_MAX(uint32) };

	using _RGBufferID = _RGResourceID;
	static constexpr _RGBufferID _RG_BUFFER_ID_NULL = _RG_RESOURCE_ID_NULL;

	using _RGTextureID = _RGResourceID;
	static constexpr _RGTextureID _RG_TEXTURE_ID_NULL = _RG_RESOURCE_ID_NULL;

	enum class PassType : uint8 {
		NONE,
		GRAPHIC,
		COMPUTE,
		TRANSFER,
		COUNT
	};

	struct RGTextureDesc {
		TextureType type = TextureType::D2;
		uint16 width = 0;
		uint16 height = 0;
		uint16 depth = 1;
		uint16 mipLevels = 1;
		TextureFormat format = TextureFormat::RGBA8;
		bool clear = false;
		ClearValue clearValue = {};
	};

	struct _RGInternalTexture {
		const char* name = nullptr;
		TextureType type = TextureType::D2;
		uint16 width = 0;
		uint16 height = 0;
		uint16 depth = 0;
		uint16 mipLevels = 0;
		TextureFormat format;
		bool clear = false;
		ClearValue clearValue = {};
	};

	struct _RGExternalTexture {
		const char* name = nullptr;
		TextureID textureID = TEXTURE_ID_NULL;
		bool clear = false;
		ClearValue clearValue = {};
	};

	struct RGBufferDesc {
		uint16 count = 0;
		uint16 typeSize = 0;
		uint16 typeAlignment = 0;
	};

	struct _RGInternalBuffer {
		const char* name = nullptr;
		uint16 count = 0;
		uint16 typeSize = 0;
		uint16 typeAlignment= 0;
		bool clear = false;
	};

	struct _RGExternalBuffer {
		const char* name = nullptr;
		BufferID bufferID = BUFFER_ID_NULL;
		bool clear = false;
	};

	struct _TextureNode {
		_RGBufferID resourceID = _RG_BUFFER_ID_NULL;
		PassNodeID writer = PASS_NODE_ID_NULL;
		Array<PassNodeID> readers;
	};

	struct _BufferNode {
		_RGBufferID resourceID = _RG_BUFFER_ID_NULL;
		PassNodeID writer = PASS_NODE_ID_NULL;
		Array<PassNodeID> readers;
	};

	namespace Command {

		class Command {
		public:
			virtual void _submit(_Database* db, ProgramID programID, VkCommandBuffer cmdBuffer) = 0;
		};

		class DrawVertex : public Command {
		public:
			GPU::ShaderArgSetID  shaderArgSets[MAX_SET_PER_SHADER_PROGRAM];
			GPU::BufferID vertexBufferID;
			uint16 vertexCount;
			void _submit(_Database* db, ProgramID programID, VkCommandBuffer cmdBuffer) final override;
		};

		class DrawIndex : public Command {
		public:

			DrawIndex() {
				for (ShaderArgSetID& argSetID : shaderArgSets) {
					argSetID = SHADER_ARG_SET_ID_NULL;
				}
			}

			ShaderArgSetID shaderArgSets[MAX_SET_PER_SHADER_PROGRAM];
			BufferID vertexBufferID = BUFFER_ID_NULL;
			BufferID indexBufferID = BUFFER_ID_NULL;
			uint16 indexCount = 0;
			void _submit(_Database* db, ProgramID programID, VkCommandBuffer cmdBuffer) final override;
		};

		class DrawIndexRegion : public Command {
		public:
			struct Viewport {
				int16 offsetX;
				int16 offsetY;
				uint16 width;
				uint16 height;
			};

			struct Scissor {
				int16 offsetX;
				int16 offsetY;
				uint16 width;
				uint16 height;
			};

			Viewport viewport;
			Scissor scissor;
			ShaderArgSetID shaderArgSets[MAX_SET_PER_SHADER_PROGRAM];
			BufferID vertexBufferID = BUFFER_ID_NULL;
			BufferID indexBufferID = BUFFER_ID_NULL;
			uint16 indexCount = 0;
			uint16 indexOffset = 0;
			uint16 vertexOffset = 0;

			void _submit(_Database* db, ProgramID program, VkCommandBuffer cmdBuffer) final override;
		};

	};

	class CommandBucket {
	public:
		uint64* keys;
		Array<Command::Command*> commands;

		void reserve(int capacity) {
			commands.resize(capacity);
		}

		template <typename Command>
		Command* put(uint32 index, uint64 key) {
			Command* command = new Command();
			commands[index] = command;
			return command;
		}

		template <typename Command>
		Command* add(uint64 key) {
			Command* command = new Command();
			commands.add(command);
			return command;
		}

		void _cleanup() {
			for (int i = 0; i < commands.size(); i++) {
				delete commands[i];
			}
			commands.cleanup();
		}
	};

	struct ColorAttachmentDesc {
		bool blendEnable = false;
		BlendFactor srcColorBlendFactor = BlendFactor::ZERO;
		BlendFactor dstColorBlendFactor = BlendFactor::ZERO;
		BlendOp colorBlendOp = BlendOp::ADD;
		BlendFactor srcAlphaBlendFactor = BlendFactor::ZERO;
		BlendFactor dstAlphaBlendFactor = BlendFactor::ZERO;
		BlendOp alphaBlendOp = BlendOp::ADD;
		bool clear = false;
		ClearValue clearValue = {};
	};

	struct ColorAttachment {
		TextureNodeID nodeID = TEXTURE_NODE_ID_NULL;
		ColorAttachmentDesc desc;

		ColorAttachment() = default;
		ColorAttachment(TextureNodeID nodeID, const ColorAttachmentDesc& desc) : nodeID(nodeID), desc(desc) {}
	};

	struct DepthStencilAttachmentDesc {
		bool depthTestEnable = false;
		bool depthWriteEnable = false;
		CompareOp depthCompareOp = CompareOp::NEVER;
		bool clear = false;
		ClearValue clearValue = {};
	};

	struct DepthStencilAttachment {
		TextureNodeID nodeID = TEXTURE_NODE_ID_NULL;
		DepthStencilAttachmentDesc desc;

		DepthStencilAttachment() = default;
		DepthStencilAttachment(TextureNodeID nodeID, const DepthStencilAttachmentDesc& desc) : nodeID(nodeID), desc(desc) {}
	};

	struct ShaderBuffer {
		BufferNodeID nodeID = BUFFER_NODE_ID_NULL;
		uint8 set = 0;
		uint8 binding = 0;

		ShaderBuffer() = default;
		ShaderBuffer(BufferNodeID nodeID, uint8 set, uint8 binding) : nodeID(nodeID), set(set), binding(binding) {}
	};

	struct ShaderTexture {
		TextureNodeID nodeID = TEXTURE_NODE_ID_NULL;
		uint8 set = 0;
		uint8 binding = 0;

		ShaderTexture() = default;
		ShaderTexture(TextureNodeID nodeID, uint8 set, uint8 binding) : nodeID(nodeID), set(set), binding(binding) {}
	};

	struct GraphicPipelineConfig {

		struct Framebuffer {
			uint16 width = 0;
			uint16 height = 0;
		} framebuffer;

		ShaderID vertexShaderID = SHADER_ID_NULL;
		ShaderID fragmentShaderID = SHADER_ID_NULL;

		struct InputLayoutConfig {
			Topology topology = Topology::TRIANGLE_LIST;
		} inputLayout;

		struct ViewportConfig {
			uint16 offsetX = 0;
			uint16 offsetY = 0;
			uint16 width = 0;
			uint16 height = 0;
		} viewport;

		struct ScissorConfig {
			bool dynamic = false;
			uint16 offsetX = 0;
			uint16 offsetY = 0;
			uint16 width = 0;
			uint16 height = 0;
		} scissor;

		struct RasterConfig {
			float lineWidth = 1.0f;
			PolygonMode polygonMode = PolygonMode::FILL;
			CullMode cullMode = CullMode::NONE;
			FrontFace frontFace = FrontFace::CLOCKWISE;
		} raster;
	};

	class PassNode {
	public:
		const char* name = nullptr;
		PassType type = PassType::NONE;
		virtual void executePass(RenderGraphRegistry* registry, CommandBucket* commandBucket) const = 0;
		virtual void cleanup() = 0;
	};

	class GraphicBaseNode : public PassNode {
	public:
		GraphicPipelineConfig pipelineConfig;
		Array<ColorAttachment> colorAttachments;
		DepthStencilAttachment depthStencilAttachment;
		Array<ShaderBuffer> inShaderBuffers;
		Array<ShaderBuffer> outShaderBuffers;
		Array<BufferNodeID> vertexBuffers;
		Array<BufferNodeID> indexBuffers;

		Array<ShaderTexture> inShaderTextures;
		Array<ShaderTexture> outShaderTextures;
		Array<TextureNodeID> inputAttachments;

		void cleanup() override {
			colorAttachments.cleanup();
			inShaderBuffers.cleanup();
			outShaderBuffers.cleanup();
			vertexBuffers.cleanup();
			indexBuffers.cleanup();

			inShaderTextures.cleanup();
			outShaderTextures.cleanup();
			inputAttachments.cleanup();
		}
	};

	template<typename Data, typename Execute>
	class GraphicNode : public GraphicBaseNode {
	public:
		Data data;
		Execute execute;

		explicit GraphicNode(Execute&& execute) : execute(std::forward<Execute>(execute)){}
		void executePass(RenderGraphRegistry* registry, CommandBucket* commandBucket) const final override {
			execute(registry, data, commandBucket);
		}
	};

	class GraphicNodeBuilder {
	public:
		PassNodeID _passID;
		GraphicBaseNode* _graphicNode;
		RenderGraph* _renderGraph;

		GraphicNodeBuilder(PassNodeID passID, GraphicBaseNode* graphicNode, RenderGraph* renderGraph):
			_passID(passID), _graphicNode(graphicNode), _renderGraph(renderGraph) {}

		GraphicNodeBuilder(const GraphicNodeBuilder& other) = delete;
		GraphicNodeBuilder(GraphicNodeBuilder&& other) = delete;

		GraphicNodeBuilder& operator=(const GraphicNodeBuilder& other) = delete;
		GraphicNodeBuilder& operator=(GraphicNodeBuilder&& other) = delete;

		~GraphicNodeBuilder(){}

		BufferNodeID addVertexBuffer(BufferNodeID nodeID);
		BufferNodeID addIndexBuffer(BufferNodeID nodeID);
		BufferNodeID addInShaderBuffer(BufferNodeID nodeID, uint8 set, uint8 binding);
		BufferNodeID addOutShaderBuffer(BufferNodeID nodeID, uint8 set, uint8 binding);

		TextureNodeID addInShaderTexture(TextureNodeID nodeID, uint8 set, uint8 binding);
		TextureNodeID addOutShaderTexture(TextureNodeID nodeID, uint8 set, uint8 binding);
		TextureNodeID addInputAttachment(TextureNodeID nodeID);
		TextureNodeID addColorAttachment(TextureNodeID nodeID, const ColorAttachmentDesc& desc);
		TextureNodeID setDepthStencilAttachment(TextureNodeID nodeID, const DepthStencilAttachmentDesc& desc);

		void setPipelineConfig(const GraphicPipelineConfig& config);
	};

	class RenderGraph {
	public:

		RenderGraph() = default;

		RenderGraph(const RenderGraph& other) = delete;
		RenderGraph(RenderGraph&& other) = delete;

		RenderGraph& operator=(const RenderGraph& other) = delete;
		RenderGraph& operator=(RenderGraph&& other) = delete;

		~RenderGraph(){}

		template<typename Data,
				SOUL_TEMPLATE_ARG_LAMBDA(Setup, void(GraphicNodeBuilder*, Data *)),
				SOUL_TEMPLATE_ARG_LAMBDA(Execute, void(RenderGraphRegistry*, const Data&, CommandBucket*))>
		const Data& addGraphicPass(const char* name, Setup setup, Execute&& execute) {
			using Node = GraphicNode<Data, Execute>;
			Node* node = (Node*) malloc(sizeof(Node));
			new(node) Node(std::forward<Execute>(execute));
			node->type = PassType::GRAPHIC;
			node->name = name;
			_passNodes.add(node);
			GraphicNodeBuilder builder(PassNodeID(_passNodes.size() - 1), node, this);
			setup(&builder, &node->data);
			return node->data;
		}

		TextureNodeID importTexture(const char* name, TextureID textureID);
		TextureNodeID createTexture(const char* name, const RGTextureDesc& desc);

		BufferNodeID importBuffer(const char* name, BufferID bufferID);
		BufferNodeID createBuffer(const char* name, const RGBufferDesc& desc);

		void cleanup();

		void _bufferNodeRead(BufferNodeID bufferNodeID, PassNodeID passNodeID);
		BufferNodeID _bufferNodeWrite(BufferNodeID bufferNodeID, PassNodeID passNodeID);

		void _textureNodeRead(TextureNodeID textureNodeID, PassNodeID passNodeID);
		TextureNodeID _textureNodeWrite(TextureNodeID textureNodeID, PassNodeID passNodeID);

		const _BufferNode* _bufferNodePtr(BufferNodeID nodeID) const ;
		_BufferNode* _bufferNodePtr(BufferNodeID nodeID);

		const _TextureNode* _textureNodePtr(TextureNodeID nodeID) const;
		_TextureNode* _textureNodePtr(TextureNodeID nodeID);

		Array<PassNode*> _passNodes;

		Array<_BufferNode> _bufferNodes;
		Array<_TextureNode> _textureNodes;

		Array<_RGInternalBuffer> _internalBuffers;
		Array<_RGInternalTexture> _internalTextures;

		Array<_RGExternalBuffer> _externalBuffers;
		Array<_RGExternalTexture> _externalTextures;

	};

}}