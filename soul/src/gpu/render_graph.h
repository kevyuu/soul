#pragma once

#include "core/type_traits.h"
#include "core/slice.h"
#include "gpu/data.h"

namespace Soul { namespace GPU {

	class RenderGraph;
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

	struct _TextureGroupNode;
	using TextureGroupNodeID = ID<_TextureGroupNode, uint16>;
	static constexpr TextureGroupNodeID TEXTURE_GROUP_NODE_ID_NULL = TextureGroupNodeID(SOUL_UTYPE_MAX(TextureGroupNodeID));

	struct _BufferGroupNode;
	using BufferGroupNodeID = ID<_BufferGroupNode, uint16>;
	static constexpr BufferGroupNodeID BUFFER_GROUP_NODE_ID_NULL = BufferGroupNodeID(SOUL_UTYPE_MAX(BufferGroupNodeID));

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
		ClearValue clearValue;
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
		TextureID textureID;
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
		BufferID bufferID;
		bool clear = false;
	};

	struct _TextureNode {
		_RGBufferID resourceID = _RG_BUFFER_ID_NULL;
		PassNodeID writer = PASS_NODE_ID_NULL;
	};

	struct _BufferNode {
		_RGBufferID resourceID = _RG_BUFFER_ID_NULL;
		PassNodeID writer = PASS_NODE_ID_NULL;
	};

	struct _BufferGroupNode {
		const char* name;
		Slice<BufferID> bufferIDs;
		PassNodeID writer = PASS_NODE_ID_NULL;
	};

	struct _TextureGroupNode {
		const char* name;
		Slice<TextureID> textureIDs;
		PassNodeID writer = PASS_NODE_ID_NULL;
	};

	class RenderCommandBucket {
	public:
		Runtime::AllocatorInitializer allocatorInitializer;
		Array<uint64> keys;
		Array<RenderCommand*> commands;

		RenderCommandBucket(): allocatorInitializer(Runtime::GetContextAllocator())
		{
			allocatorInitializer.end();
		}

		void reserve(int capacity) {
			keys.resize(capacity);
			commands.resize(capacity);
		}

		template <typename Command>
		Command* put(uint32 index, uint64 key) {
			Command* command = Runtime::GetTempAllocator()->create<Command>();
			keys[index] = key;
			commands[index] = command;
			return command;
		}

		template <typename Command>
		Command* add(uint64 key) {
			Command* command = Runtime::GetTempAllocator()->create<Command>();
			commands.add(command);
			return command;
		}

	};

	struct ColorAttachmentDesc {
		bool clear = false;
		ClearValue clearValue;
	};

	struct ColorAttachment {
		TextureNodeID nodeID = TEXTURE_NODE_ID_NULL;
		ColorAttachmentDesc desc;

		ColorAttachment() = default;
		ColorAttachment(TextureNodeID nodeID, const ColorAttachmentDesc& desc) : nodeID(nodeID), desc(desc) {}
	};

	struct DepthStencilAttachmentDesc {
		bool depthWriteEnable;
		bool clear = false;
		ClearValue clearValue;
	};

	struct DepthStencilAttachment {
		TextureNodeID nodeID = TEXTURE_NODE_ID_NULL;
		DepthStencilAttachmentDesc desc;

		DepthStencilAttachment() = default;
		DepthStencilAttachment(TextureNodeID nodeID, const DepthStencilAttachmentDesc& desc) : nodeID(nodeID), desc(desc) {}
	};

    struct InputAttachment {
        TextureNodeID nodeID = TEXTURE_NODE_ID_NULL;
		ShaderStageFlags stageFlags;

        InputAttachment() = default;
        InputAttachment(TextureNodeID nodeID, ShaderStageFlags stageFlags) : nodeID(nodeID), stageFlags(stageFlags) {}
    };

	enum class ShaderBufferReadUsage : uint8 {
		UNIFORM,
		STORAGE,
		COUNT
	};

	struct ShaderBufferReadAccess {
		BufferNodeID nodeID = BUFFER_NODE_ID_NULL;
		ShaderStageFlags stageFlags;
		ShaderBufferReadUsage usage;
	};

	enum class ShaderBufferWriteUsage : uint8 {
		UNIFORM,
		COUNT
	};

	struct ShaderBufferWriteAccess {
		BufferNodeID nodeID = BUFFER_NODE_ID_NULL;
		ShaderStageFlags stageFlags;
		ShaderBufferWriteUsage usage;
	};

	enum class ShaderTextureReadUsage : uint8 {
		UNIFORM,
		STORAGE,
		COUNT
	};

	struct ShaderTextureReadAccess {
		TextureNodeID nodeID = TEXTURE_NODE_ID_NULL;
		ShaderStageFlags stageFlags;
		ShaderTextureReadUsage usage;
	};

	enum class ShaderTextureWriteUsage : uint8 {
		STORAGE,
		COUNT
	};

	struct ShaderTextureWriteAccess {
		TextureNodeID nodeID = TEXTURE_NODE_ID_NULL;
		ShaderStageFlags stageFlags;
		ShaderTextureWriteUsage usage;
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

	struct ComputePipelineConfig {
	    ShaderID computeShaderID = SHADER_ID_NULL;
	};

	class PassNode {
	public:

		PassNode() = default;
		PassNode(const PassNode&) = delete;
		PassNode& operator=(const PassNode&) = delete;
		PassNode(PassNode&&) = delete;
		PassNode& operator=(PassNode&&) = delete;
		virtual ~PassNode() = default;

		const char* name = nullptr;
		PassType type = PassType::NONE;
		virtual void executePass(RenderGraphRegistry* registry, RenderCommandBucket* commandBucket) const = 0;
		virtual void cleanup() = 0;
	};

	class GraphicBaseNode : public PassNode {
	public:

		GraphicBaseNode() = default;
		GraphicBaseNode(const GraphicBaseNode&) = delete;
		GraphicBaseNode& operator=(const GraphicBaseNode&) = delete;
		GraphicBaseNode(GraphicBaseNode&&) = delete;
		GraphicBaseNode& operator=(GraphicBaseNode&&) = delete;
		~GraphicBaseNode() override {};

		Vec2ui32 renderTargetDimension;

		Array<ColorAttachment> colorAttachments;
		DepthStencilAttachment depthStencilAttachment;
		Array<ShaderBufferReadAccess> shaderBufferReadAccesses;
		Array<ShaderBufferWriteAccess> shaderBufferWriteAccesses;
		Array<BufferNodeID> vertexBuffers;
		Array<BufferNodeID> indexBuffers;

		Array<ShaderTextureReadAccess> shaderTextureReadAccesses;
		Array<ShaderTextureWriteAccess> shaderTextureWriteAccesses;
		Array<InputAttachment> inputAttachments;

		Array<BufferGroupNodeID> vertexBufferGroups;
		Array<BufferGroupNodeID> indexBufferGroups;
		Array<BufferGroupNodeID> inShaderBufferGroups;
		Array<TextureGroupNodeID> inShaderTextureGroups;

		void cleanup() override {}
	};

	class ComputeBaseNode : public PassNode {
	public:
	    ComputePipelineConfig pipelineConfig;
		Array<ShaderBufferReadAccess> shaderBufferReadAccesses;
		Array<ShaderBufferWriteAccess> shaderBufferWriteAccesses;
		Array<ShaderTextureReadAccess> shaderTextureReadAccesses;
		Array<ShaderTextureWriteAccess> shaderTextureWriteAccesses;

		Array<BufferGroupNodeID> inShaderBufferGroups;
		Array<TextureGroupNodeID> inShaderTextureGroups;

	    void cleanup() override {
	        //
	    }
	};

	class TransferBaseNode : public PassNode {
	public:

		void cleanup() override {
			//
		}
	};

	template<typename Data, typename Execute>
	class GraphicNode : public GraphicBaseNode {
	public:
		Data data;
		Execute execute;

		GraphicNode() = delete;
		GraphicNode(const GraphicNode&) = delete;
		GraphicNode& operator=(const GraphicNode&) = delete;
		GraphicNode(GraphicNode&&) = delete;
		GraphicNode& operator=(GraphicNode&&) = delete;
		~GraphicNode() override{}

		explicit GraphicNode(Execute&& execute) : execute(std::forward<Execute>(execute)){}
		void executePass(RenderGraphRegistry* registry, RenderCommandBucket* commandBucket) const final override {
			execute(registry, data, commandBucket);
		}
	};

	template<typename Data, typename Execute>
	class ComputeNode : public ComputeBaseNode {
	public:
	    Data data;
	    Execute execute;

        explicit ComputeNode(Execute&& execute) : execute(std::forward<Execute>(execute)){}
        void executePass(RenderGraphRegistry* registry, RenderCommandBucket* commandBucket) const final override {
            execute(registry, data, commandBucket);
        }
	};

	template<typename Data, typename Execute>
	class TransferNode : TransferBaseNode {
	public:
		Data data;
		Execute execute;
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

		~GraphicNodeBuilder() = default;

		BufferNodeID addVertexBuffer(BufferNodeID nodeID);
		BufferNodeID addIndexBuffer(BufferNodeID nodeID);

		BufferNodeID addShaderBuffer(BufferNodeID nodeID, ShaderStageFlags stageFlags, ShaderBufferReadUsage usageType);
		BufferNodeID addShaderBuffer(BufferNodeID nodeID, ShaderStageFlags stageFlags, ShaderBufferWriteUsage usageType);

		TextureNodeID addShaderTexture(TextureNodeID nodeID, ShaderStageFlags stageFlags, ShaderTextureReadUsage usageType);
		TextureNodeID addShaderTexture(TextureNodeID nodeID, ShaderStageFlags stageFlags, ShaderTextureWriteUsage usageType);

		TextureNodeID addInputAttachment(TextureNodeID nodeID, ShaderStageFlags stageFlags);
		TextureNodeID addColorAttachment(TextureNodeID nodeID, const ColorAttachmentDesc& desc);
		TextureNodeID setDepthStencilAttachment(TextureNodeID nodeID, const DepthStencilAttachmentDesc& desc);
		void setRenderTargetDimension(Vec2ui32 dimension);

	};

	class ComputeNodeBuilder {
	public:
	    PassNodeID _passID;
	    ComputeBaseNode* _computeNode;
	    RenderGraph* _renderGraph;

        ComputeNodeBuilder(PassNodeID passID, ComputeBaseNode* computeNode, RenderGraph* renderGraph):
        _passID(passID), _computeNode(computeNode), _renderGraph(renderGraph) {}

        ComputeNodeBuilder(const ComputeNodeBuilder& other) = delete;
        ComputeNodeBuilder(ComputeNodeBuilder&& other) = delete;

        ComputeNodeBuilder& operator=(const ComputeNodeBuilder& other) = delete;
        ComputeNodeBuilder& operator=(ComputeNodeBuilder&& other) = delete;

		~ComputeNodeBuilder() = default;

		BufferNodeID addShaderBuffer(BufferNodeID nodeID, ShaderStageFlags stageFlags, ShaderBufferReadUsage usageType);
		BufferNodeID addShaderBuffer(BufferNodeID nodeID, ShaderStageFlags stageFlags, ShaderBufferWriteUsage usageType);

		TextureNodeID addShaderTexture(TextureNodeID nodeID, ShaderStageFlags stageFlags, ShaderTextureReadUsage usageType);
		TextureNodeID addShaderTexture(TextureNodeID nodeID, ShaderStageFlags stageFlags, ShaderTextureWriteUsage usageType);

        BufferNodeID addInShaderBuffer(BufferNodeID nodeID, uint8 set, uint8 binding);
        BufferNodeID addOutShaderBuffer(BufferNodeID nodeID, uint8 set, uint8 binding);

        TextureNodeID addInShaderTexture(TextureNodeID nodeID, uint8 set, uint8 binding);
        TextureNodeID addOutShaderTexture(TextureNodeID nodeID, uint8 set, uint8 binding);

        void setPipelineConfig(const ComputePipelineConfig& config);
	};

	struct TextureExport {
		TextureNodeID tex;
		char* pixels;
	};

	class RenderGraph {
	public:

		RenderGraph() = default;

		RenderGraph(const RenderGraph& other) = delete;
		RenderGraph(RenderGraph&& other) = delete;

		RenderGraph& operator=(const RenderGraph& other) = delete;
		RenderGraph& operator=(RenderGraph&& other) = delete;

		~RenderGraph()
		{
			
		}

		template<
			typename DATA,
			typename SETUP,
			typename EXECUTE,
			SOUL_REQUIRE(is_lambda_v<SETUP, void(GraphicNodeBuilder*, DATA*)>),
			SOUL_REQUIRE(is_lambda_v<EXECUTE, void(RenderGraphRegistry*, const DATA&, RenderCommandBucket*)>)
		>
		const DATA& addGraphicPass(const char* name, SETUP setup, EXECUTE&& execute) {
			using Node = GraphicNode<DATA, EXECUTE>;
			Node* node = (Node*) malloc(sizeof(Node));
			new(node) Node(std::forward<EXECUTE>(execute));
			node->type = PassType::GRAPHIC;
			node->name = name;
			_passNodes.add(node);
			GraphicNodeBuilder builder(PassNodeID(_passNodes.size() - 1), node, this);
			setup(&builder, &node->data);
			return node->data;
		}

		template<
			typename DATA,
			typename SETUP,
			typename EXECUTE,
			SOUL_REQUIRE(is_lambda_v<SETUP, void(GraphicNodeBuilder*, DATA*)>),
			SOUL_REQUIRE(is_lambda_v<EXECUTE, void(RenderGraphRegistry*, const DATA&, RenderCommandBucket*)>)
		>
        const DATA& addComputePass(const char* name, SETUP setup, EXECUTE&& execute) {
            using Node = ComputeNode<DATA, EXECUTE>;
            Node* node = (Node*) malloc(sizeof(Node));
            new(node) Node(std::forward<EXECUTE>(execute));
            node->type = PassType::COMPUTE;
            node->name = name;
            _passNodes.add(node);
            ComputeNodeBuilder builder(PassNodeID(_passNodes.size() - 1), node, this);
            setup(&builder, &node->data);
            return node->data;
		}

		TextureNodeID importTexture(const char* name, TextureID textureID);
		TextureNodeID createTexture(const char* name, const RGTextureDesc& desc);

		BufferNodeID importBuffer(const char* name, BufferID bufferID);
		BufferNodeID createBuffer(const char* name, const RGBufferDesc& desc);

		TextureGroupNodeID importTextureGroup(const char* name, Slice<TextureID> textureIDs);
		BufferGroupNodeID importBufferGroup(const char* name, Slice<BufferID> bufferIDs);

		void exportTexture(TextureNodeID tex, char* pixels);

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

		Array<TextureExport> _textureExports;

	};

}}