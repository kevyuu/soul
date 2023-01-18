#pragma once

#include "core/type_traits.h"
#include "core/slice.h"

#include "memory/allocator.h"

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
	class PassBaseNode;
	class RGShaderPassDependencyBuilder;
	class RGTransferPassDependencyBuilder;

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
		std::invocable<Func, const Data&, gpu::RenderGraphRegistry&, gpu::TransferCommandList&> &&
		std::same_as<void, std::invoke_result_t<Func, const Data&, gpu::RenderGraphRegistry&, gpu::TransferCommandList&>>;

	template <typename Func, typename Data>
	concept shader_pass_setup =
		std::invocable<Func, RGShaderPassDependencyBuilder&, Data&> &&
		std::same_as<void, std::invoke_result_t<Func, gpu::RGShaderPassDependencyBuilder&, Data&>>;

	template <typename Func, typename Data>
	concept copy_pass_setup =
		std::invocable<Func, RGTransferPassDependencyBuilder&, Data&> &&
		std::same_as<void, std::invoke_result_t<Func, gpu::RGTransferPassDependencyBuilder&, Data&>>;

	//ID
	using PassNodeID = ID<PassBaseNode, uint16>;

	using TextureNodeID = ID<impl::TextureNode, uint16>;

	using BufferNodeID = ID<impl::BufferNode, uint16>;

	struct RGTextureDesc
	{
		TextureType type = TextureType::D2;
		TextureFormat format = TextureFormat::RGBA8;
		vec3ui32 extent;
		uint32 mip_levels = 1;
		uint16 layer_count = 1;
		TextureSampleCount sample_count = gpu::TextureSampleCount::COUNT_1;
		bool clear = false;
		ClearValue clear_value;

		static RGTextureDesc create_d2(const TextureFormat format, const uint32 mip_levels, 
			const vec2ui32 dimension, bool clear = false, ClearValue clear_value = {}, 
			const TextureSampleCount sample_count = TextureSampleCount::COUNT_1)
		{
			return {
				.type = TextureType::D2,
				.format = format,
				.extent = vec3ui32(dimension.x, dimension.y, 1),
				.mip_levels = mip_levels,
				.layer_count = 1,
				.sample_count = sample_count,
				.clear = clear,
				.clear_value = clear_value
			};
		}

		static RGTextureDesc create_d3(const TextureFormat format, const uint32 mip_levels,
			const vec3ui32 dimension, bool clear = false, ClearValue clear_value = {},
			const TextureSampleCount sample_count = TextureSampleCount::COUNT_1)
		{
			return {
				.type = TextureType::D3,
				.format = format,
				.extent = dimension,
				.mip_levels = mip_levels,
				.layer_count = 1,
				.sample_count = sample_count,
				.clear = clear,
				.clear_value = clear_value
			};
		}

		static RGTextureDesc create_d2_array(const TextureFormat format, const uint32 mip_levels, 
			const vec2ui32 dimension, const uint16 layer_count, bool clear = false, 
			ClearValue clear_value = {})
		{
			return {
				.type = TextureType::D2_ARRAY,
				.format = format,
				.extent = vec3ui32(dimension.x, dimension.y, 1),
				.mip_levels = mip_levels,
				.layer_count = layer_count,
				.clear = clear,
				.clear_value = clear_value
			};
		}
	};

	struct RGBufferDesc
	{
		soul_size size = 0;
	};

	struct ColorAttachmentDesc
	{
		TextureNodeID node_id;
		SubresourceIndex view = SubresourceIndex();
		bool clear = false;
		ClearValue clear_value;
	};

	struct DepthStencilAttachmentDesc
	{
		TextureNodeID node_id;
		SubresourceIndex view;
		bool depth_write_enable = true;
		bool clear = false;
		ClearValue clear_value;
	};

	struct ResolveAttachmentDesc
	{
		TextureNodeID node_id;
		SubresourceIndex view;
		bool clear = false;
		ClearValue clear_value;
	};

	enum class ShaderBufferReadUsage : uint8
	{
		UNIFORM,
		STORAGE,
		COUNT
	};

	struct ShaderBufferReadAccess
	{
		BufferNodeID node_id;
		ShaderStageFlags stage_flags;
		ShaderBufferReadUsage usage;
	};

	enum class ShaderBufferWriteUsage : uint8
	{
		UNIFORM,
		STORAGE,
		COUNT
	};

	struct ShaderBufferWriteAccess
	{
		BufferNodeID input_node_id;
		BufferNodeID output_node_id;
		ShaderStageFlags stage_flags;
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
		TextureNodeID node_id;
		ShaderStageFlags stage_flags;
		ShaderTextureReadUsage usage;
		SubresourceIndexRange view_range;
	};

	enum class ShaderTextureWriteUsage : uint8
	{
		STORAGE,
		COUNT
	};

	struct ShaderTextureWriteAccess
	{
		TextureNodeID input_node_id;
		TextureNodeID output_node_id;
		ShaderStageFlags stage_flags;
		ShaderTextureWriteUsage usage;
		SubresourceIndexRange view_range;
	};

	template <typename AttachmentDesc>
	struct AttachmentAccess
	{
		TextureNodeID out_node_id;
		AttachmentDesc desc;
	};
	using ColorAttachment = AttachmentAccess<ColorAttachmentDesc>;
	using DepthStencilAttachment = AttachmentAccess<DepthStencilAttachmentDesc>;
	using ResolveAttachment = AttachmentAccess<ResolveAttachmentDesc>;

	struct TransferSrcBufferAccess
	{
		BufferNodeID node_id;
	};

	enum class TransferDataSource
	{
	    GPU,
		CPU,
		COUNT
	};

	struct TransferDstBufferAccess
	{
		TransferDataSource data_source;
		BufferNodeID input_node_id;
		BufferNodeID output_node_id;
	};

	struct TransferSrcTextureAccess
	{
		TextureNodeID node_id;
		SubresourceIndexRange view_range;
	};

	struct TransferDstTextureAccess
	{
		TransferDataSource data_source;
		TextureNodeID input_node_id;
		TextureNodeID output_node_id;
		SubresourceIndexRange view_range;
	};

	struct RGRenderTargetDesc
	{
		RGRenderTargetDesc() = default;

		RGRenderTargetDesc(vec2ui32 dimension, const ColorAttachmentDesc& color) : dimension(dimension)
		{
			color_attachments.push_back(color);
		}

		RGRenderTargetDesc(vec2ui32 dimension, const ColorAttachmentDesc& color, const DepthStencilAttachmentDesc& depth_stencil) :
			dimension(dimension), depth_stencil_attachment(depth_stencil)
		{
			color_attachments.push_back(color);
		}

		RGRenderTargetDesc(vec2ui32 dimension, const TextureSampleCount sample_count, const ColorAttachmentDesc& color, 
			const ResolveAttachmentDesc& resolve, const DepthStencilAttachmentDesc depth_stencil):
			dimension(dimension), sample_count(sample_count), depth_stencil_attachment(depth_stencil)
		{
			color_attachments.push_back(color);
			resolve_attachments.push_back(resolve);
		}

		RGRenderTargetDesc(vec2ui32 dimension, const DepthStencilAttachmentDesc& depth_stencil) :
			dimension(dimension), depth_stencil_attachment(depth_stencil)  {}

		vec2ui32 dimension;
		TextureSampleCount sample_count = TextureSampleCount::COUNT_1;
		Vector<ColorAttachmentDesc> color_attachments;
		Vector<ResolveAttachmentDesc> resolve_attachments;
		DepthStencilAttachmentDesc depth_stencil_attachment;
	};

	struct RGRenderTarget
	{
		vec2ui32 dimension;
		TextureSampleCount sample_count = TextureSampleCount::COUNT_1;
		Vector<ColorAttachment> color_attachments;
		Vector<ResolveAttachment> resolve_attachments;
		DepthStencilAttachment depth_stencil_attachment;
	};

	enum class RGBuilderFlag : uint8
	{
		RENDER_TARGET,
		SHADER_RESOURCE,
		TRANSFER,
		COUNT
	};

	using RGBuilderFlags = FlagSet<RGBuilderFlag>;

	template <RGBuilderFlags BuilderFlags>
	class RGDependencyBuilder
	{

	private:
		RGDependencyBuilder(const PassNodeID pass_id, PassBaseNode& pass_node, RenderGraph& render_graph);
		const PassNodeID pass_id_;
		PassBaseNode& pass_node_;
		RenderGraph& render_graph_;

	public:

		BufferNodeID add_shader_buffer(BufferNodeID node_id, ShaderStageFlags stage_flags,
			ShaderBufferReadUsage usage_type);
		BufferNodeID add_shader_buffer(BufferNodeID node_id, ShaderStageFlags stage_flags,
			ShaderBufferWriteUsage usage_type);
		TextureNodeID add_shader_texture(TextureNodeID node_id, ShaderStageFlags stage_flags,
			ShaderTextureReadUsage usage_type, SubresourceIndexRange view = SubresourceIndexRange());
		TextureNodeID add_shader_texture(TextureNodeID node_id, ShaderStageFlags stage_flags,
			ShaderTextureWriteUsage usage_type, SubresourceIndexRange view = SubresourceIndexRange());
		BufferNodeID add_vertex_buffer(BufferNodeID node_id);
		BufferNodeID add_index_buffer(BufferNodeID node_id);

		BufferNodeID add_src_buffer(BufferNodeID node_id);
		BufferNodeID add_dst_buffer(BufferNodeID node_id, TransferDataSource data_source = TransferDataSource::GPU);
		TextureNodeID add_src_texture(TextureNodeID node_id);
		TextureNodeID add_dst_texture(TextureNodeID node_id, TransferDataSource data_source = TransferDataSource::GPU);
	};

	template<RGBuilderFlags BuilderFlags>
	inline BufferNodeID RGDependencyBuilder<BuilderFlags>::add_shader_buffer(BufferNodeID node_id, ShaderStageFlags stage_flags, ShaderBufferReadUsage usage_type)
	{
		static_assert(BuilderFlags.test(RGBuilderFlag::SHADER_RESOURCE));
		render_graph_.read_buffer_node(node_id, pass_id_);
		pass_node_.shader_buffer_read_accesses_.push_back({ node_id, stage_flags, usage });
		return node_id;
	}

	template<RGBuilderFlags BuilderFlags>
	inline BufferNodeID RGDependencyBuilder<BuilderFlags>::add_shader_buffer(BufferNodeID node_id, ShaderStageFlags stage_flags, ShaderBufferWriteUsage usage_type)
	{
		static_assert(BuilderFlags.test(RGBuilderFlag::SHADER_RESOURCE));
		const auto out_node_id = render_graph_.write_buffer_node(node_id, pass_id_);
		pass_node_.shader_buffer_write_accesses_.push_back({ node_id, out_node_id, stage_flags, usage });
		return out_node_id;
	}

	template<RGBuilderFlags BuilderFlags>
	inline TextureNodeID RGDependencyBuilder<BuilderFlags>::add_shader_texture(TextureNodeID node_id, ShaderStageFlags stage_flags, ShaderTextureReadUsage usage_type, SubresourceIndexRange view)
	{
		static_assert(BuilderFlags.test(RGBuilderFlag::SHADER_RESOURCE));
		render_graph_.read_texture_node(node_id, pass_id_);
		pass_node_.shader_texture_read_accesses_.push_back({ node_id, stage_flags, usage, view_range });
		return node_id;
	}

	template<RGBuilderFlags BuilderFlags>
	inline TextureNodeID RGDependencyBuilder<BuilderFlags>::add_shader_texture(TextureNodeID node_id, ShaderStageFlags stage_flags, ShaderTextureWriteUsage usage_type, SubresourceIndexRange view)
	{
		static_assert(BuilderFlags.test(RGBuilderFlag::SHADER_RESOURCE));
		const auto out_node_id = render_graph_.write_texture_node(node_id, pass_id);
		pass_node_.shader_texture_write_accesses_.push_back({ node_id, out_node_id, stage_flags, usage, view_range });
		return out_node_id;
	}

	template<RGBuilderFlags BuilderFlags>
	inline BufferNodeID RGDependencyBuilder<BuilderFlags>::add_vertex_buffer(BufferNodeID node_id)
	{
		render_graph_.read_buffer_node(node_id, pass_id_);
		pass_node_.vertex_buffers_.push_back(node_id);
		return node_id;
	}

	template<RGBuilderFlags BuilderFlags>
	inline BufferNodeID RGDependencyBuilder<BuilderFlags>::add_index_buffer(BufferNodeID node_id)
	{
		render_graph_.read_buffer_node(node_id, pass_id_);
		pass_node_.index_buffers_.push_back(node_id);
		return node_id;
	}

	class PassBaseNode
	{
	public:
		PassBaseNode(const PassType type, const char* name) : name_(name), type_(type) {}
		PassBaseNode(const PassBaseNode&) = delete;
		PassBaseNode& operator=(const PassBaseNode&) = delete;
		PassBaseNode(PassBaseNode&&) = delete;
		PassBaseNode& operator=(PassBaseNode&&) = delete;
		virtual ~PassBaseNode() = default;

		[[nodiscard]] PassType get_type() const { return type_; }
		[[nodiscard]] const char* get_name() const { return name_; }

		[[nodiscard]] const Vector<BufferNodeID>& get_vertex_buffers() const { return vertex_buffers_; }
		[[nodiscard]] const Vector<BufferNodeID>& get_index_buffers() const { return index_buffers_; }
		[[nodiscard]] const Vector<ShaderBufferReadAccess>& get_buffer_read_accesses() const { return shader_buffer_read_accesses_; }
		[[nodiscard]] const Vector<ShaderBufferWriteAccess>& get_buffer_write_accesses() const { return shader_buffer_write_accesses_; }
		[[nodiscard]] const Vector<ShaderTextureReadAccess>& get_texture_read_accesses() const { return shader_texture_read_accesses_; }
		[[nodiscard]] const Vector<ShaderTextureWriteAccess>& get_texture_write_accesses() const { return shader_texture_write_accesses_; }

		[[nodiscard]] const RGRenderTarget* get_render_target() const { return &render_target_.value(); }

	protected:
		const char* name_ = nullptr;
		const PassType type_ = PassType::NONE;
		const QueueType queue_type = QueueType::COUNT;

	private:

		Vector<TransferSrcBufferAccess> source_buffers_;
		Vector<TransferDstBufferAccess> destination_buffers_;
		Vector<TransferSrcTextureAccess> source_textures_;
		Vector<TransferDstTextureAccess> destination_textures_;

		Vector<ShaderBufferReadAccess> shader_buffer_read_accesses_;
		Vector<ShaderBufferWriteAccess> shader_buffer_write_accesses_;
		Vector<ShaderTextureReadAccess> shader_texture_read_accesses_;
		Vector<ShaderTextureWriteAccess> shader_texture_write_accesses_;
		Vector<BufferNodeID> vertex_buffers_;
		Vector<BufferNodeID> index_buffers_;

		std::optional<RGRenderTarget> render_target_;

		template <RGBuilderFlags BuilderFlags>
		friend class RGDependencyBuilder;
	};

	template <typename Parameter,
        typename Execute>
	class PassNode final : public PassBaseNode
	{
	public:
		PassNode() = delete;
		PassNode(const PassNode&) = delete;
		PassNode& operator=(const PassNode&) = delete;
		PassNode(PassNode&&) = delete;
		PassNode& operator=(PassNode&&) = delete;
		~PassNode() override = default;

		PassNode(const char* name, Execute&& execute) : PassBaseNode(PassType::GRAPHIC, name), execute_(std::forward<Execute>(execute)) {}

		[[nodiscard]] const Parameter& get_parameter() const { return parameter_; }
		const Execute& get_execute() { return execute_; }
		
	private:
		Parameter parameter_;
		Execute execute_;
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
			typename Node = PassNode<Parameter, Executable>
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
					.out_node_id = write_texture_node(attachment_desc.node_id, pass_node_id),
					.desc = attachment_desc
				};
				return access;
			};
			
			std::ranges::transform(render_target.color_attachments,
				std::back_inserter(node->render_target_.color_attachments), to_output_attachment);
			std::ranges::transform(render_target.resolve_attachments,
				std::back_inserter(node->render_target_.resolve_attachments), to_output_attachment);
			if (render_target.depth_stencil_attachment.node_id.is_valid())
			{
				const auto& depth_desc = render_target.depth_stencil_attachment;
				const TextureNodeID out_node_id = depth_desc.depth_write_enable ? depth_desc.node_id : write_texture_node(depth_desc.node_id, pass_node_id);
				node->render_target_.depth_stencil_attachment = {
					.out_node_id = out_node_id,
					.desc = depth_desc
				};
			}
			node->render_target_.dimension = render_target.dimension;
			node->render_target_.sample_count = render_target.sample_count;

			return *node;
		}

		template <
			typename Parameter,
			shader_pass_setup<Parameter&> Setup,
			compute_pass_executable<Parameter&> Executable,
			typename Node = PassNode<Parameter, Executable>
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
			typename Node = PassNode<Parameter, Executable>
		>
		const Node& add_transfer_pass(const char* name, Setup&& setup, Executable&& execute)
		{
			auto node = allocator_->create<Node>(name, std::forward<Executable>(execute));
			pass_nodes_.push_back(node);
			RGTransferPassDependencyBuilder builder(PassNodeID(pass_nodes_.size() - 1), *node, *this);
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

		[[nodiscard]] const Vector<PassBaseNode*>& get_pass_nodes() const { return pass_nodes_; }
		[[nodiscard]] const Vector<impl::BufferNode>& get_buffer_nodes() const { return buffer_nodes_; }
		[[nodiscard]] const Vector<impl::TextureNode>& get_texture_nodes() const { return texture_nodes_; }
		[[nodiscard]] const Vector<impl::RGInternalBuffer>& get_internal_buffers() const { return internal_buffers_; }
		[[nodiscard]] const Vector<impl::RGInternalTexture>& get_internal_textures() const { return internal_textures_; }
		[[nodiscard]] const Vector<impl::RGExternalBuffer>& get_external_buffers() const { return external_buffers_; }
		[[nodiscard]] const Vector<impl::RGExternalTexture>& get_external_textures() const { return external_textures_; }

		[[nodiscard]] RGTextureDesc get_texture_desc(TextureNodeID node_id, const gpu::System& gpu_system) const;
		[[nodiscard]] RGBufferDesc get_buffer_desc(BufferNodeID node_id, const gpu::System& gpu_system) const;

	private:
		Vector<PassBaseNode*> pass_nodes_;

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
			vec3ui32 extent;
			uint32 mip_levels = 1;
			uint16 layer_count = 1;
			TextureSampleCount sample_count = TextureSampleCount::COUNT_1;
			bool clear = false;
			ClearValue clear_value = {};

			[[nodiscard]] soul_size get_view_count() const
			{
				return soul::cast<uint64>(mip_levels) * layer_count;
			}
		};

		struct RGExternalTexture
		{
			const char* name = nullptr;
			TextureID texture_id;
			bool clear = false;
			ClearValue clear_value = {};
		};

		struct RGInternalBuffer
		{
			const char* name = nullptr;
			soul_size size = 0;
			bool clear = false;
		};

		struct RGExternalBuffer
		{
			const char* name = nullptr;
			BufferID buffer_id;
			bool clear = false;
		};

		struct TextureNode
		{
			impl::RGBufferID resource_id;
			PassNodeID creator;
			PassNodeID writer;
			TextureNodeID write_target_node;
			Vector<PassNodeID> readers;
		};

		struct BufferNode
		{
			impl::RGBufferID resource_id;
			PassNodeID creator;
			PassNodeID writer;
			BufferNodeID write_target_node;
			Vector<PassNodeID> readers;
		};
	}
}
