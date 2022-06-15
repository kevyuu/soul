#pragma once

#include "core/type_traits.h"
#include "core/slice.h"

#include "memory/allocator.h"
#include "runtime/runtime.h"

#include "gpu/type.h"
#include "gpu/command_list.h"

namespace soul::gpu
{
	namespace impl
	{
		struct TextureNode;
		struct BufferNode;
		struct RGResourceID;

		struct RGResourceID
		{
			uint32 index = std::numeric_limits<uint32>::max();

			static constexpr uint8 EXTERNAL_BIT_POSITION = 31;

			static RGResourceID internal_id(const uint32 index) { return {index}; }
			static RGResourceID external_id(const uint32 index) { return {index | 1u << EXTERNAL_BIT_POSITION}; }

			[[nodiscard]] bool is_external() const
			{
				return index & (1u << EXTERNAL_BIT_POSITION);
			}

			[[nodiscard]] uint32 get_index() const
			{
				return index & ~(1u << EXTERNAL_BIT_POSITION);
			}
		};

		constexpr RGResourceID RG_RESOURCE_ID_NULL = {std::numeric_limits<uint32>::max()};

		using RGBufferID = RGResourceID;
		constexpr RGBufferID RG_BUFFER_ID_NULL = RG_RESOURCE_ID_NULL;

		using RGTextureID = RGResourceID;
		constexpr RGTextureID RG_TEXTURE_ID_NULL = RG_RESOURCE_ID_NULL;

		struct RGInternalTexture;
		struct RGExternalTexture;
		struct RGInternalBuffer;
		struct RGExternalBuffer;
	}

	class RenderGraph;
	class RenderGraphRegistry;
	class PassNode;
	class RGShaderPassDependencyBuilder;
	class RGCopyPassDependencyBuilder;

	template <typename Func, typename Data>
	concept graphic_pass_executable =
		std::invocable<Func, const Data&, gpu::RenderGraphRegistry&, gpu::GraphicCommandList&> &&
		std::same_as<void, std::invoke_result_t<Func, const Data&, gpu::RenderGraphRegistry&, gpu::GraphicCommandList&>>;

	template <typename Func, typename Data>
	concept compute_pass_executable =
		std::invocable<Func, const Data&, gpu::RenderGraphRegistry&, gpu::ComputeCommandList&> &&
		std::same_as<void, std::invoke_result_t<Func, const Data&, gpu::RenderGraphRegistry&, gpu::ComputeCommandList&>>;

	template <typename Func, typename Data>
	concept copy_pass_executable =
		std::invocable<Func, const Data&, gpu::RenderGraphRegistry&, gpu::CopyCommandList&> &&
		std::same_as<void, std::invoke_result_t<Func, const Data&, gpu::RenderGraphRegistry&, gpu::CopyCommandList&>>;

	template <typename Func, typename Data>
	concept shader_pass_setup =
		std::invocable<Func, RGShaderPassDependencyBuilder&, Data&> &&
		std::same_as<void, std::invoke_result_t<Func, gpu::RGShaderPassDependencyBuilder&, Data&>>;

	template <typename Func, typename Data>
	concept copy_pass_setup =
		std::invocable<Func, RGCopyPassDependencyBuilder&, Data&> &&
		std::same_as<void, std::invoke_result_t<Func, gpu::RGCopyPassDependencyBuilder&, Data&>>;

	//ID
	using PassNodeID = ID<PassNode, uint16>;

	using TextureNodeID = ID<impl::TextureNode, uint16>;

	using BufferNodeID = ID<impl::BufferNode, uint16>;

	enum class PassType : uint8
	{
		NONE,
		GRAPHIC,
		COMPUTE,
		TRANSFER,
		COUNT
	};

	struct RGTextureDesc
	{
		TextureType type = TextureType::D2;
		TextureFormat format = TextureFormat::RGBA8;
		Vec3ui32 extent;
		uint32 mipLevels = 1;
		uint16 layerCount = 1;
		TextureSampleCount sampleCount = gpu::TextureSampleCount::COUNT_1;
		bool clear = false;
		ClearValue clearValue;

		static RGTextureDesc create_d2(const TextureFormat format, const uint32 mip_levels, const Vec2ui32 dimension, bool clear = false, ClearValue clear_value = {}, const TextureSampleCount sample_count = TextureSampleCount::COUNT_1)
		{
			return {
				.type = TextureType::D2,
				.format = format,
				.extent = Vec3ui32(dimension.x, dimension.y, 1),
				.mipLevels = mip_levels,
				.layerCount = 1,
				.sampleCount = sample_count,
				.clear = clear,
				.clearValue = clear_value
			};
		}

		static RGTextureDesc create_d2_array(const TextureFormat format, const uint32 mip_levels, const Vec2ui32 dimension, const uint16 layer_count, bool clear = false, ClearValue clear_value = {})
		{
			return {
				.type = TextureType::D2_ARRAY,
				.format = format,
				.extent = Vec3ui32(dimension.x, dimension.y, 1),
				.mipLevels = mip_levels,
				.layerCount = layer_count,
				.clear = clear,
				.clearValue = clear_value
			};
		}
	};

	struct RGBufferDesc
	{
		soul_size count = 0;
		uint16 typeSize = 0;
		uint16 typeAlignment = 0;
	};

	struct ColorAttachmentDesc
	{
		TextureNodeID nodeID;
		SubresourceIndex view = SubresourceIndex();
		bool clear = false;
		ClearValue clearValue;
	};

	struct DepthStencilAttachmentDesc
	{
		TextureNodeID nodeID;
		SubresourceIndex view;
		bool depthWriteEnable = true;
		bool clear = false;
		ClearValue clearValue;
	};

	struct ResolveAttachmentDesc
	{
		TextureNodeID nodeID;
		SubresourceIndex view;
		bool clear = false;
		ClearValue clearValue;
	};

	enum class ShaderBufferReadUsage : uint8
	{
		UNIFORM,
		STORAGE,
		COUNT
	};

	struct ShaderBufferReadAccess
	{
		BufferNodeID nodeID;
		ShaderStageFlags stageFlags;
		ShaderBufferReadUsage usage;
	};

	enum class ShaderBufferWriteUsage : uint8
	{
		UNIFORM,
		COUNT
	};

	struct ShaderBufferWriteAccess
	{
		BufferNodeID inputNodeID;
		BufferNodeID outputNodeID;
		ShaderStageFlags stageFlags;
		ShaderBufferWriteUsage usage;
	};

	enum class ShaderTextureReadUsage : uint8
	{
		UNIFORM,
		STORAGE,
		COUNT
	};

	struct ShaderTextureReadAccess
	{
		TextureNodeID nodeID;
		ShaderStageFlags stageFlags;
		ShaderTextureReadUsage usage;
		SubresourceIndexRange viewRange;
	};

	enum class ShaderTextureWriteUsage : uint8
	{
		STORAGE,
		COUNT
	};

	struct ShaderTextureWriteAccess
	{
		TextureNodeID inputNodeID;
		TextureNodeID outputNodeID;
		ShaderStageFlags stageFlags;
		ShaderTextureWriteUsage usage;
		SubresourceIndexRange viewRange;
	};

	template <typename AttachmentDesc>
	struct AttachmentAccess
	{
		TextureNodeID outNodeID;
		AttachmentDesc desc;
	};
	using ColorAttachment = AttachmentAccess<ColorAttachmentDesc>;
	using DepthStencilAttachment = AttachmentAccess<DepthStencilAttachmentDesc>;
	using ResolveAttachment = AttachmentAccess<ResolveAttachmentDesc>;

	struct TransferSrcBufferAccess
	{
		BufferNodeID nodeID;
	};

	struct TransferDstBufferAccess
	{
		BufferNodeID inputNodeID;
		BufferNodeID outputNodeID;
	};

	struct TransferSrcTextureAccess
	{
		TextureNodeID nodeID;
		SubresourceIndexRange viewRange;
	};

	struct TransferDstTextureAccess
	{
		TextureNodeID inputNodeID;
		TextureNodeID outputNodeID;
		SubresourceIndexRange viewRange;
	};

	struct RGRenderTargetDesc
	{
		RGRenderTargetDesc() = default;

		RGRenderTargetDesc(Vec2ui32 dimension, const ColorAttachmentDesc& color) : dimension(dimension)
		{
			colorAttachments.push_back(color);
		}

		RGRenderTargetDesc(Vec2ui32 dimension, const ColorAttachmentDesc& color, const DepthStencilAttachmentDesc& depth_stencil) :
			dimension(dimension), depthStencilAttachment(depth_stencil)
		{
			colorAttachments.push_back(color);
		}

		RGRenderTargetDesc(Vec2ui32 dimension, const TextureSampleCount sample_count, const ColorAttachmentDesc& color, 
			const ResolveAttachmentDesc& resolve, const DepthStencilAttachmentDesc depth_stencil):
			dimension(dimension), sampleCount(sample_count), depthStencilAttachment(depth_stencil)
		{
			colorAttachments.push_back(color);
			resolveAttachments.push_back(resolve);
		}

		RGRenderTargetDesc(Vec2ui32 dimension, const DepthStencilAttachmentDesc& depth_stencil) :
			dimension(dimension), depthStencilAttachment(depth_stencil)  {}

		Vec2ui32 dimension;
		TextureSampleCount sampleCount = TextureSampleCount::COUNT_1;
		Vector<ColorAttachmentDesc> colorAttachments;
		Vector<ResolveAttachmentDesc> resolveAttachments;
		DepthStencilAttachmentDesc depthStencilAttachment;
	};

	struct RGRenderTarget
	{
		Vec2ui32 dimension;
		TextureSampleCount sampleCount = TextureSampleCount::COUNT_1;
		Vector<ColorAttachment> colorAttachments;
		Vector<ResolveAttachment> resolveAttachments;
		DepthStencilAttachment depthStencilAttachment;
	};

	class PassNode
	{
	public:
		PassNode(const PassType type, const char* name) : name_(name), type_(type) {}
		PassNode(const PassNode&) = delete;
		PassNode& operator=(const PassNode&) = delete;
		PassNode(PassNode&&) = delete;
		PassNode& operator=(PassNode&&) = delete;
		virtual ~PassNode() = default;

		[[nodiscard]] PassType get_type() const { return type_; }
		[[nodiscard]] const char* get_name() const { return name_; }

	protected:
		const char* name_ = nullptr;
		const PassType type_ = PassType::NONE;
	};

	class RGShaderPassDependencyBuilder;
	class ShaderNode : public PassNode
	{
	private:
		Vector<ShaderBufferReadAccess> shader_buffer_read_accesses_;
		Vector<ShaderBufferWriteAccess> shader_buffer_write_accesses_;
		Vector<ShaderTextureReadAccess> shader_texture_read_accesses_;
		Vector<ShaderTextureWriteAccess> shader_texture_write_accesses_;
		Vector<BufferNodeID> vertex_buffers_;
		Vector<BufferNodeID> index_buffers_;

		friend class RGShaderPassDependencyBuilder;

	public:
		ShaderNode(const PassType type, const char* name) : PassNode(type, name) {}
		ShaderNode(const ShaderNode&) = delete;
		ShaderNode& operator=(const ShaderNode&) = delete;
		ShaderNode(ShaderNode&&) = delete;
		ShaderNode& operator=(ShaderNode&&) = delete;
		~ShaderNode() override = default;

		[[nodiscard]] const Vector<BufferNodeID>& get_vertex_buffers() const { return vertex_buffers_;  }
		[[nodiscard]] const Vector<BufferNodeID>& get_index_buffers() const { return index_buffers_; }
		[[nodiscard]] const Vector<ShaderBufferReadAccess>& get_buffer_read_accesses() const { return shader_buffer_read_accesses_; }
		[[nodiscard]] const Vector<ShaderBufferWriteAccess>& get_buffer_write_accesses() const { return shader_buffer_write_accesses_; }
		[[nodiscard]] const Vector<ShaderTextureReadAccess>& get_texture_read_accesses() const { return shader_texture_read_accesses_; }
		[[nodiscard]] const Vector<ShaderTextureWriteAccess>& get_texture_write_accesses() const { return shader_texture_write_accesses_;  }
	};

	class GraphicBaseNode : public ShaderNode
	{
	public:
		explicit GraphicBaseNode(const char* name) : ShaderNode(PassType::GRAPHIC, name) {}
		GraphicBaseNode(const GraphicBaseNode&) = delete;
		GraphicBaseNode& operator=(const GraphicBaseNode&) = delete;
		GraphicBaseNode(GraphicBaseNode&&) = delete;
		GraphicBaseNode& operator=(GraphicBaseNode&&) = delete;
		~GraphicBaseNode() override = default;
		virtual void execute_pass(RenderGraphRegistry& registry, GraphicCommandList& command_list) = 0;
		[[nodiscard]] const RGRenderTarget& get_render_target() const { return render_target_; }
	private:
		RGRenderTarget render_target_;
		friend class RenderGraph;
	};

	template <typename Parameter,
		graphic_pass_executable<Parameter> Execute>
	class GraphicNode final : public GraphicBaseNode
	{
		Parameter parameter_;
		Execute execute_;

	public:
		GraphicNode() = delete;
		GraphicNode(const GraphicNode&) = delete;
		GraphicNode& operator=(const GraphicNode&) = delete;
		GraphicNode(GraphicNode&&) = delete;
		GraphicNode& operator=(GraphicNode&&) = delete;
		~GraphicNode() override = default;

		GraphicNode(const char* name, Execute&& execute) : GraphicBaseNode(name) , execute_(std::forward<Execute>(execute)) {}

		void execute_pass(RenderGraphRegistry& registry, GraphicCommandList& command_list) override
		{
			execute_(parameter_, registry, command_list);
		}

		[[nodiscard]] const Parameter& get_parameter() const { return parameter_; }
		Parameter& get_parameter_() { return parameter_; }

		friend class RenderGraph;

	};

	class ComputeBaseNode : public ShaderNode
	{
	public:
		explicit ComputeBaseNode(const char* name) : ShaderNode(PassType::COMPUTE, name) {}
		ComputeBaseNode(const ComputeBaseNode&) = delete;
		ComputeBaseNode& operator=(const ComputeBaseNode&) = delete;
		ComputeBaseNode(ComputeBaseNode&&) = delete;
		ComputeBaseNode& operator=(ComputeBaseNode&&) = delete;
		~ComputeBaseNode() override = default;

		virtual void execute_pass(RenderGraphRegistry& registry, ComputeCommandList& command_list) = 0;
	};

	template <typename Parameter,
		compute_pass_executable<Parameter> Execute>
	class ComputeNode final : public ComputeBaseNode
	{
		Parameter parameter_;
		Execute execute_;

	public:

		ComputeNode() = delete;
		ComputeNode(const ComputeNode&) = delete;
		ComputeNode& operator=(const ComputeNode&) = delete;
		ComputeNode(ComputeNode&&) = delete;
		ComputeNode& operator=(ComputeNode&&) = delete;
		~ComputeNode() override = default;

		ComputeNode(const char* name, Execute&& execute) : ComputeBaseNode(name) , execute_(std::forward<Execute>(execute)) {}

		void execute_pass(RenderGraphRegistry& registry, ComputeCommandList& command_list) override
		{
			execute_(parameter_, registry, command_list);
		}
		[[nodiscard]] const Parameter& get_parameter() const { return parameter_; }
		Parameter& get_parameter_() { return parameter_; }
		
	};

	class CopyBaseNode : public ShaderNode
	{
	public:
		explicit CopyBaseNode(const char* name) : ShaderNode(PassType::TRANSFER, name) {}
		CopyBaseNode(const CopyBaseNode&) = delete;
		CopyBaseNode& operator=(const CopyBaseNode&) = delete;
		CopyBaseNode(CopyBaseNode&&) = delete;
		CopyBaseNode& operator=(CopyBaseNode&&) = delete;
		~CopyBaseNode() override = default;

		const Vector<TransferSrcBufferAccess>& get_source_buffers() const { return source_buffers_; }
		const Vector<TransferDstBufferAccess>& get_destination_buffers() const { return destination_buffers_; }
		const Vector<TransferSrcTextureAccess>& get_source_textures() const { return source_textures_; }
		const Vector<TransferDstTextureAccess>& get_destination_textures() const { return destination_textures_; }

		virtual void execute_pass(RenderGraphRegistry& registry, CopyCommandList& command_list) = 0;

		friend class RGCopyPassDependencyBuilder;
	private:
		Vector<TransferSrcBufferAccess> source_buffers_;
		Vector<TransferDstBufferAccess> destination_buffers_;
		Vector<TransferSrcTextureAccess> source_textures_;
		Vector<TransferDstTextureAccess> destination_textures_;
	};

	template <typename Parameter,
		copy_pass_executable<Parameter> Execute>
	class CopyNode final : public CopyBaseNode
	{
		Parameter parameter_;
		Execute execute_;
	public:
		CopyNode() = delete;
		CopyNode(const CopyNode&) = delete;
		CopyNode& operator=(const CopyNode&) = delete;
		CopyNode(CopyNode&&) = delete;
		CopyNode& operator=(CopyNode&&) = delete;
		~CopyNode() override = default;

		CopyNode(const char* name, Execute&& execute) : CopyBaseNode(name), execute_(std::forward<Execute>(execute)) {}

		void execute_pass(RenderGraphRegistry& registry, CopyCommandList& command_list) override
		{
			execute_(parameter_, registry, command_list);
		}
		const Parameter& get_parameter() const { return parameter_; }
		Parameter& get_parameter_() { return parameter_; }
	};

	class RGShaderPassDependencyBuilder
	{
	private:
		const PassNodeID pass_id_;
		ShaderNode& shader_node_;
		RenderGraph& render_graph_;

	public:

		RGShaderPassDependencyBuilder(const PassNodeID pass_id, ShaderNode& shader_node, RenderGraph& render_graph)
			: pass_id_(pass_id), shader_node_(shader_node), render_graph_(render_graph) {}
		RGShaderPassDependencyBuilder(const RGShaderPassDependencyBuilder&) = delete;
		RGShaderPassDependencyBuilder& operator=(const RGShaderPassDependencyBuilder&) = delete;
		RGShaderPassDependencyBuilder(RGShaderPassDependencyBuilder&&) = delete;
		RGShaderPassDependencyBuilder& operator=(RGShaderPassDependencyBuilder&&) = delete;
		~RGShaderPassDependencyBuilder() = default;

		BufferNodeID add_shader_buffer(const BufferNodeID node_id, const ShaderStageFlags stage_flags, 
			const ShaderBufferReadUsage usage_type);
		BufferNodeID add_shader_buffer(const BufferNodeID node_id, const ShaderStageFlags stage_flags,
			const ShaderBufferWriteUsage usage_type);
		TextureNodeID add_shader_texture(const TextureNodeID node_id, const ShaderStageFlags stage_flags,
			const ShaderTextureReadUsage usage_type, const SubresourceIndexRange view = SubresourceIndexRange());
		TextureNodeID add_shader_texture(const TextureNodeID node_id, const ShaderStageFlags stage_flags,
			const ShaderTextureWriteUsage usage_type, const SubresourceIndexRange view = SubresourceIndexRange());
		BufferNodeID add_vertex_buffer(const BufferNodeID node_id);
		BufferNodeID add_index_buffer(const BufferNodeID node_id);
	};
	using RGGraphicPassDependencyBuilder = RGShaderPassDependencyBuilder;
	using RGComputePassDependencyBuilder = RGShaderPassDependencyBuilder;

	class RGCopyPassDependencyBuilder
	{
		const PassNodeID pass_id_;
		CopyBaseNode& copy_base_node_;
		RenderGraph& render_graph_;
	public:
		RGCopyPassDependencyBuilder(const PassNodeID pass_id, CopyBaseNode& copy_base_node, RenderGraph& render_graph)
			: pass_id_(pass_id), copy_base_node_(copy_base_node), render_graph_(render_graph) {}
		RGCopyPassDependencyBuilder(const RGCopyPassDependencyBuilder&) = delete;
		RGCopyPassDependencyBuilder& operator=(const RGCopyPassDependencyBuilder&) = delete;
		RGCopyPassDependencyBuilder(RGCopyPassDependencyBuilder&&) = delete;
		RGCopyPassDependencyBuilder& operator=(RGCopyPassDependencyBuilder&&) = delete;
		~RGCopyPassDependencyBuilder() = default;

		BufferNodeID add_src_buffer(const BufferNodeID node_id);
		BufferNodeID add_dst_buffer(const BufferNodeID node_id);
		TextureNodeID add_src_texture(const TextureNodeID node_id);
		TextureNodeID add_dst_texture(const TextureNodeID node_id);
	};


	class RenderGraph
	{
	public:
		explicit RenderGraph(memory::Allocator* allocator = get_default_allocator()) : allocator_(allocator) {}
		RenderGraph(const RenderGraph& other) = delete;
		RenderGraph(RenderGraph&& other) = delete;
		RenderGraph& operator=(const RenderGraph& other) = delete;
		RenderGraph& operator=(RenderGraph&& other) = delete;
		~RenderGraph();

		template <
			typename Parameter,
			shader_pass_setup<Parameter&> Setup,
			graphic_pass_executable<Parameter&> Executable,
			typename Node = GraphicNode<Parameter, Executable>
		>
		const Node& add_graphic_pass(const char* name, const RGRenderTargetDesc& render_target, Setup&& setup, Executable&& execute)
		{
			auto* node = allocator_->create<Node>(name, std::forward<Executable>(execute));
			
			pass_nodes_.push_back(node);
			const auto pass_node_id = PassNodeID(pass_nodes_.size() - 1);
			RGShaderPassDependencyBuilder builder(pass_node_id, *node, *this);
			setup(builder, node->get_parameter_());

			auto to_output_attachment = [this, pass_node_id]<typename AttachmentDesc>(const AttachmentDesc attachment_desc) -> auto
			{
				AttachmentAccess<AttachmentDesc> access = {
					.outNodeID = write_texture_node(attachment_desc.nodeID, pass_node_id),
					.desc = attachment_desc
				};
				return access;
			};
			
			std::ranges::transform(render_target.colorAttachments,
				std::back_inserter(node->render_target_.colorAttachments), to_output_attachment);
			std::ranges::transform(render_target.resolveAttachments,
				std::back_inserter(node->render_target_.resolveAttachments), to_output_attachment);
			if (render_target.depthStencilAttachment.nodeID.is_valid())
			{
				const auto& depth_desc = render_target.depthStencilAttachment;
				const TextureNodeID out_node_id = depth_desc.depthWriteEnable ? depth_desc.nodeID : write_texture_node(depth_desc.nodeID, pass_node_id);
				node->render_target_.depthStencilAttachment = {
					.outNodeID = out_node_id,
					.desc = depth_desc
				};
			}
			node->render_target_.dimension = render_target.dimension;
			node->render_target_.sampleCount = render_target.sampleCount;

			return *node;
		}

		template <
			typename Parameter,
			shader_pass_setup<Parameter&> Setup,
			compute_pass_executable<Parameter&> Executable,
			typename Node = ComputeNode<Parameter, Executable>
		>
		const Node& add_compute_pass(const char* name, Setup&& setup, Executable&& execute)
		{
			auto node = allocator_->create<Node>(name, std::forward<Executable>(execute));
			pass_nodes_.push_back(node);
			RGShaderPassDependencyBuilder builder(PassNodeID(pass_nodes_.size() - 1), *node, *this);
			setup(builder, node->get_parameter_());
			return *node;
		}

		template<
			typename Parameter,
			copy_pass_setup<Parameter&> Setup,
			copy_pass_executable<Parameter&> Executable,
			typename Node = CopyNode<Parameter, Executable>
		>
		const Node& add_copy_pass(const char* name, Setup&& setup, Executable&& execute)
		{
			auto node = allocator_->create<Node>(name, std::forward<Executable>(execute));
			pass_nodes_.push_back(node);
			RGCopyPassDependencyBuilder builder(PassNodeID(pass_nodes_.size() - 1), *node, *this);
			setup(builder, node->get_parameter_());
			return *node;
		}
		
		TextureNodeID import_texture(const char* name, TextureID texture_id);
		TextureNodeID create_texture(const char* name, const RGTextureDesc& desc);
		
		BufferNodeID import_buffer(const char* name, BufferID buffer_id);
		BufferNodeID create_buffer(const char* name, const RGBufferDesc& desc);
		

		void cleanup();

		void read_buffer_node(BufferNodeID buffer_node_id, PassNodeID pass_node_id);
		BufferNodeID write_buffer_node(BufferNodeID buffer_node_id, PassNodeID pass_node_id);

		void read_texture_node(TextureNodeID texture_node_id, PassNodeID pass_node_id);
		TextureNodeID write_texture_node(TextureNodeID texture_node_id, PassNodeID pass_node_id);

		[[nodiscard]] const impl::BufferNode& get_buffer_node(BufferNodeID node_id) const;
		impl::BufferNode& get_buffer_node(BufferNodeID node_id);

		[[nodiscard]] const impl::TextureNode& get_texture_node(TextureNodeID node_id) const;
		impl::TextureNode& get_texture_node(TextureNodeID node_id);

		[[nodiscard]] const Vector<PassNode*>& get_pass_nodes() const { return pass_nodes_; }
		[[nodiscard]] const Vector<impl::BufferNode>& get_buffer_nodes() const { return buffer_nodes_; }
		[[nodiscard]] const Vector<impl::TextureNode>& get_texture_nodes() const { return texture_nodes_; }
		[[nodiscard]] const Vector<impl::RGInternalBuffer>& get_internal_buffers() const { return internal_buffers_; }
		[[nodiscard]] const Vector<impl::RGInternalTexture>& get_internal_textures() const { return internal_textures_; }
		[[nodiscard]] const Vector<impl::RGExternalBuffer>& get_external_buffers() const { return external_buffers_; }
		[[nodiscard]] const Vector<impl::RGExternalTexture>& get_external_textures() const { return external_textures_; }

		[[nodiscard]] RGTextureDesc get_texture_desc(TextureNodeID node_id, const gpu::System& gpu_system) const;
		[[nodiscard]] RGBufferDesc get_buffer_desc(BufferNodeID node_id, const gpu::System& gpu_system) const;

	private:
		Vector<PassNode*> pass_nodes_;

		Vector<impl::BufferNode> buffer_nodes_;
		Vector<impl::TextureNode> texture_nodes_;

		Vector<impl::RGInternalBuffer> internal_buffers_;
		Vector<impl::RGInternalTexture> internal_textures_;

		Vector<impl::RGExternalBuffer> external_buffers_;
		Vector<impl::RGExternalTexture> external_textures_;
		
		memory::Allocator* allocator_;
	};

	namespace impl
	{
		struct RGInternalTexture
		{
			const char* name = nullptr;
			TextureType type = TextureType::D2;
			TextureFormat format = TextureFormat::COUNT;
			Vec3ui32 extent;
			uint32 mipLevels = 1;
			uint16 layerCount = 1;
			TextureSampleCount sampleCount = TextureSampleCount::COUNT_1;
			bool clear = false;
			ClearValue clearValue = {};

			[[nodiscard]] soul_size get_view_count() const
			{
				return mipLevels * layerCount;
			}
		};

		struct RGExternalTexture
		{
			const char* name = nullptr;
			TextureID textureID;
			bool clear = false;
			ClearValue clearValue = {};
		};

		struct RGInternalBuffer
		{
			const char* name = nullptr;
			uint16 count = 0;
			uint16 typeSize = 0;
			uint16 typeAlignment = 0;
			bool clear = false;
		};

		struct RGExternalBuffer
		{
			const char* name = nullptr;
			BufferID bufferID;
			bool clear = false;
		};

		struct TextureNode
		{
			impl::RGBufferID resourceID;
			PassNodeID creator;
			PassNodeID writer;
			TextureNodeID writeTargetNode;
			Vector<PassNodeID> readers;
		};

		struct BufferNode
		{
			impl::RGBufferID resourceID;
			PassNodeID creator;
			PassNodeID writer;
			BufferNodeID writeTargetNode;
			Vector<PassNodeID> readers;
		};
	}
}
