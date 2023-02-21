#pragma once

#include "core/type_traits.h"
#include "core/slice.h"

#include "memory/allocator.h"

#include "gpu/type.h"
#include "gpu/command_list.h"

#include <span>

namespace soul::gpu
{
	namespace impl
	{
        struct ResourceNode;
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
		class RenderGraphExecution;
	}

	class RenderGraph;
	class RenderGraphRegistry;
	class PassBaseNode;
	template <PipelineFlags pipeline_flags>
	class RGDependencyBuilder;

	template <typename Func, typename Data, PipelineFlags pipeline_flags>
	concept pass_setup =
		std::invocable<Func, Data&, RGDependencyBuilder<pipeline_flags>&> &&
		std::same_as<void, std::invoke_result_t<Func, Data&, RGDependencyBuilder<pipeline_flags>&>>;

	template <typename Func, typename Data>
	concept non_shader_pass_setup = pass_setup<Func, Data, PIPELINE_FLAGS_NON_SHADER>;

	template <typename Func, typename Data>
	concept raster_pass_setup = pass_setup<Func, Data, PIPELINE_FLAGS_RASTER>;

	template <typename Func, typename Data>
	concept compute_pass_setup = pass_setup<Func, Data, PIPELINE_FLAGS_COMPUTE>;

	
	template <typename Func, typename Data, PipelineFlags pipeline_flags>
	concept pass_executable = std::invocable<Func, const Data&, gpu::RenderGraphRegistry&, CommandList<pipeline_flags>&> &&
		std::same_as<void, std::invoke_result_t<Func, const Data&, gpu::RenderGraphRegistry&, CommandList<pipeline_flags>&>>;

	template <typename Func, typename Data>
	concept non_shader_pass_executable = pass_executable < Func, Data, PIPELINE_FLAGS_NON_SHADER>;

    template <typename Func, typename Data>
	concept raster_pass_executable = pass_executable <Func, Data, PIPELINE_FLAGS_RASTER>;

	template <typename Func, typename Data>
	concept compute_pass_executable = pass_executable <Func, Data, PIPELINE_FLAGS_COMPUTE>;

	//ID
	using PassNodeID = ID<PassBaseNode, uint16>;
	using ResourceNodeID = ID<impl::ResourceNode, uint16>;

	enum class RGResourceType : uint8
	{
	    BUFFER,
		TEXTURE,
		COUNT
	};

	template<RGResourceType resource_type>
	struct TypedResourceNodeID
	{
		using this_type = TypedResourceNodeID<resource_type>;
		ResourceNodeID id;

		friend auto operator<=>(const this_type&, const this_type&) = default;
		[[nodiscard]] bool is_null() const { return id.is_null(); }
		[[nodiscard]] bool is_valid() const { return id.is_valid(); }
	};
	using BufferNodeID = TypedResourceNodeID<RGResourceType::BUFFER>;
	using TextureNodeID = TypedResourceNodeID<RGResourceType::TEXTURE>;

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

	enum class TransferDataSource : uint8
	{
	    GPU,
		CPU,
		COUNT
	};

	struct TransferDstBufferAccess
	{
		TransferDataSource data_source = TransferDataSource::COUNT;
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
		TransferDataSource data_source = TransferDataSource::COUNT;
		TextureNodeID input_node_id;
		TextureNodeID output_node_id;
		SubresourceIndexRange view_range;
	};

	struct RGRenderTargetDesc
	{
		RGRenderTargetDesc() = default;

		RGRenderTargetDesc(vec2ui32 dimension, const ColorAttachmentDesc& color) : 
			dimension(dimension), color_attachments(std::to_array({color})){}

		RGRenderTargetDesc(vec2ui32 dimension, const ColorAttachmentDesc& color, const DepthStencilAttachmentDesc& depth_stencil) :
			dimension(dimension), color_attachments(std::to_array({color})), depth_stencil_attachment(depth_stencil) {}

		RGRenderTargetDesc(vec2ui32 dimension, const TextureSampleCount sample_count, const ColorAttachmentDesc& color, 
			const ResolveAttachmentDesc& resolve, const DepthStencilAttachmentDesc depth_stencil):
			dimension(dimension), sample_count(sample_count), 
			color_attachments(std::to_array({color})), resolve_attachments(std::to_array({resolve})), depth_stencil_attachment(depth_stencil) {}

		RGRenderTargetDesc(vec2ui32 dimension, const DepthStencilAttachmentDesc& depth_stencil) :
			dimension(dimension), depth_stencil_attachment(depth_stencil)  {}

		vec2ui32 dimension = {};
		TextureSampleCount sample_count = TextureSampleCount::COUNT_1;
		SBOVector<ColorAttachmentDesc, 1> color_attachments;
		SBOVector<ResolveAttachmentDesc, 1> resolve_attachments;
		DepthStencilAttachmentDesc depth_stencil_attachment;
	};

	struct RGRenderTarget
	{
		vec2ui32 dimension = {};
		TextureSampleCount sample_count = TextureSampleCount::COUNT_1;
		Vector<ColorAttachment> color_attachments;
		Vector<ResolveAttachment> resolve_attachments;
		DepthStencilAttachment depth_stencil_attachment;
	};
	
	template <PipelineFlags pipeline_flags>
	class RGDependencyBuilder
	{
	public:
		static constexpr auto PIPELINE_FLAGS = pipeline_flags;
		RGDependencyBuilder(PassNodeID pass_id, PassBaseNode& pass_node, RenderGraph& render_graph);
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

		void set_render_target(const RGRenderTargetDesc& render_target_desc);

	private:
		const PassNodeID pass_id_;
		PassBaseNode& pass_node_;
		RenderGraph& render_graph_;

	};

	class PassBaseNode
	{
	public:
		PassBaseNode(const char* name, const PipelineFlags pipeline_flags, const QueueType queue_type) :
	        name_(name), pipeline_flags_(pipeline_flags), queue_type_(queue_type) {}
		PassBaseNode(const PassBaseNode&) = delete;
		PassBaseNode& operator=(const PassBaseNode&) = delete;
		PassBaseNode(PassBaseNode&&) = delete;
		PassBaseNode& operator=(PassBaseNode&&) = delete;
		virtual ~PassBaseNode() = default;
		
		[[nodiscard]] const char* get_name() const { return name_; }
		[[nodiscard]] PipelineFlags get_pipeline_flags() const { return pipeline_flags_; }
		[[nodiscard]] QueueType get_queue_type() const { return queue_type_; }

		[[nodiscard]] std::span<const BufferNodeID> get_vertex_buffers() const { return vertex_buffers_; }
		[[nodiscard]] std::span<const BufferNodeID> get_index_buffers() const { return index_buffers_; }
		[[nodiscard]] std::span<const ShaderBufferReadAccess> get_buffer_read_accesses() const { return shader_buffer_read_accesses_; }
		[[nodiscard]] std::span<const ShaderBufferWriteAccess> get_buffer_write_accesses() const { return shader_buffer_write_accesses_; }
		[[nodiscard]] std::span<const ShaderTextureReadAccess> get_texture_read_accesses() const { return shader_texture_read_accesses_; }
		[[nodiscard]] std::span<const ShaderTextureWriteAccess> get_texture_write_accesses() const { return shader_texture_write_accesses_; }
		[[nodiscard]] std::span<const TransferSrcBufferAccess> get_source_buffers() const { return source_buffers_; }
		[[nodiscard]] std::span<const TransferDstBufferAccess> get_destination_buffers() const { return destination_buffers_; }
		[[nodiscard]] std::span<const TransferSrcTextureAccess> get_source_textures() const { return source_textures_; }
		[[nodiscard]] std::span<const TransferDstTextureAccess> get_destination_textures() const { return destination_textures_; }

		[[nodiscard]] const RGRenderTarget& get_render_target() const { return render_target_; }
		[[nodiscard]] TextureNodeID get_color_attachment_node_id() const { return render_target_.color_attachments[0].out_node_id; }

	protected:
		const char* name_ = nullptr;
		const PipelineFlags pipeline_flags_;
		const QueueType queue_type_;

	private:

		virtual void execute(
			RenderGraphRegistry& registry,
			impl::RenderCompiler& render_compiler,
			const VkRenderPassBeginInfo* render_pass_begin_info,
			impl::CommandPools& command_pools,
			System& gpu_system) const = 0;

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

		RGRenderTarget render_target_;

		template <PipelineFlags pipeline_flags>
		friend class RGDependencyBuilder;

		friend class impl::RenderGraphExecution;
	};

	template <
		PipelineFlags pipeline_flags,
		typename Parameter,
        typename Execute
    >
	class PassNode final : public PassBaseNode
	{
	public:
		PassNode() = delete;
		PassNode(const PassNode&) = delete;
		PassNode& operator=(const PassNode&) = delete;
		PassNode(PassNode&&) = delete;
		PassNode& operator=(PassNode&&) = delete;
		~PassNode() override = default;

		PassNode(const char* name, const QueueType queue_type, Execute&& execute) :
	        PassBaseNode(name, pipeline_flags, queue_type), execute_(std::forward<Execute>(execute)) {}

		[[nodiscard]] const Parameter& get_parameter() const { return parameter_; }
		void execute(
			RenderGraphRegistry& render_graph_execution,
			impl::RenderCompiler& render_compiler,
			const VkRenderPassBeginInfo* render_pass_begin_info,
			impl::CommandPools& command_pools,
			System& gpu_system) const override;
		
	private:
		Parameter& get_parameter() { return parameter_; }

		Parameter parameter_;
		Execute execute_;
		friend class RenderGraph;
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

		template<
			typename Parameter,
			PipelineFlags pipeline_flags,
			typename Setup,
			typename Executable
		>
		const auto& add_pass(const char* name, QueueType queue_type, Setup&& setup, Executable&& execute)
		{
			static_assert(pass_setup<Setup, Parameter, pipeline_flags>);
			static_assert(pass_executable<Executable, Parameter, pipeline_flags>);

			using Node = PassNode<pipeline_flags, Parameter, Executable>;
			auto node = allocator_->create<Node>(name, queue_type, std::forward<Executable>(execute));
			pass_nodes_.push_back(node);
			RGDependencyBuilder<pipeline_flags> builder(PassNodeID(pass_nodes_.size() - 1), *node, *this);
			setup(node->get_parameter(), builder);
			return *node;
		}

		template <
			typename Parameter,
			typename Setup,
			typename Executable
		>
		const auto& add_raster_pass(const char* name, const RGRenderTargetDesc& render_target, Setup&& setup, Executable&& execute)
		{
			static_assert(raster_pass_setup <Setup, Parameter>);
			static_assert(raster_pass_executable<Executable, Parameter>);

			constexpr auto pipeline_flags = PipelineFlags{ PipelineType::RASTER };
			using Node = PassNode<pipeline_flags, Parameter, Executable>;
			auto* node = allocator_->create<Node>(name, QueueType::GRAPHIC, std::forward<Executable>(execute));
			pass_nodes_.push_back(node);
			const auto pass_node_id = PassNodeID(pass_nodes_.size() - 1);
		    RGDependencyBuilder<pipeline_flags> builder(pass_node_id, *node, *this);
			builder.set_render_target(render_target);
			setup(node->get_parameter(), builder);

			return *node;
		}

		template <
			typename Parameter,
			typename Setup,
			typename Executable
		>
		const auto& add_compute_pass(const char* name, Setup&& setup, Executable&& execute)
		{
			static_assert(compute_pass_setup <Setup, Parameter>);
			static_assert(compute_pass_executable<Executable, Parameter>);

			return add_pass<Parameter, PIPELINE_FLAGS_COMPUTE>(name, QueueType::COMPUTE, 
				std::forward<Setup>(setup), std::forward<Executable>(execute));
		}

		template<
			typename Parameter,
			typename Setup,
			typename Executable
		>
		const auto& add_non_shader_pass(const char* name, QueueType queue_type, Setup&& setup, Executable&& execute)
		{
			static_assert(non_shader_pass_setup <Setup, Parameter>);
			static_assert(non_shader_pass_executable<Executable, Parameter>);

			return add_pass<Parameter, PIPELINE_FLAGS_NON_SHADER>(name, queue_type, 
				std::forward<Setup>(setup), std::forward<Executable>(execute));
		}
		
		TextureNodeID import_texture(const char* name, TextureID texture_id);
		TextureNodeID create_texture(const char* name, const RGTextureDesc& desc);
		
		BufferNodeID import_buffer(const char* name, BufferID buffer_id);
		BufferNodeID create_buffer(const char* name, const RGBufferDesc& desc);

		[[nodiscard]] const Vector<PassBaseNode*>& get_pass_nodes() const { return pass_nodes_; }
		[[nodiscard]] const Vector<impl::RGInternalBuffer>& get_internal_buffers() const { return internal_buffers_; }
		[[nodiscard]] const Vector<impl::RGInternalTexture>& get_internal_textures() const { return internal_textures_; }
		[[nodiscard]] const Vector<impl::RGExternalBuffer>& get_external_buffers() const { return external_buffers_; }
		[[nodiscard]] const Vector<impl::RGExternalTexture>& get_external_textures() const { return external_textures_; }

		[[nodiscard]] RGTextureDesc get_texture_desc(TextureNodeID node_id, const gpu::System& gpu_system) const;
		[[nodiscard]] RGBufferDesc get_buffer_desc(BufferNodeID node_id, const gpu::System& gpu_system) const;

	private:
		Vector<PassBaseNode*> pass_nodes_;

		Vector<impl::ResourceNode> resource_nodes_;

		Vector<impl::RGInternalBuffer> internal_buffers_;
		Vector<impl::RGInternalTexture> internal_textures_;

		Vector<impl::RGExternalBuffer> external_buffers_;
		Vector<impl::RGExternalTexture> external_textures_;
		
		memory::Allocator* allocator_;

		friend class impl::RenderGraphExecution;
		template<PipelineFlags pipeline_flags>
		friend class RGDependencyBuilder;

		ResourceNodeID create_resource_node(RGResourceType resource_type, impl::RGResourceID resource_id);
		void read_resource_node(ResourceNodeID resource_node_id, PassNodeID pass_node_id);
		ResourceNodeID write_resource_node(ResourceNodeID resource_node_id, PassNodeID pass_node_id);
		[[nodiscard]] const impl::ResourceNode& get_resource_node(ResourceNodeID node_id) const;
		impl::ResourceNode& get_resource_node(ResourceNodeID node_id);
		[[nodiscard]] std::span<const impl::ResourceNode> get_resource_nodes() const;

		template<RGResourceType resource_type>
		TypedResourceNodeID<resource_type> create_resource_node(impl::RGResourceID resource_id)
		{
			return {create_resource_node(resource_type, resource_id)};
		}

		template <RGResourceType resource_type>
		void read_resource_node(TypedResourceNodeID<resource_type> node_id, PassNodeID pass_node_id)
		{
			read_resource_node(node_id.id, pass_node_id);
		}

		template<RGResourceType resource_type>
		TypedResourceNodeID<resource_type> write_resource_node(TypedResourceNodeID<resource_type> resource_node_id, PassNodeID pass_node_id)
		{
			return { write_resource_node(resource_node_id.id, pass_node_id) };
		}

		template <RGResourceType resource_type>
	    const impl::ResourceNode& get_resource_node(TypedResourceNodeID<resource_type> node_id) const
		{
			return get_resource_node(node_id.id);
		}

	};

	namespace impl
	{
		struct RGInternalTexture
		{
			const char* name = nullptr;
			TextureType type = TextureType::D2;
			TextureFormat format = TextureFormat::COUNT;
			vec3ui32 extent = {};
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

		struct ResourceNode
		{
			RGResourceType resource_type;
			impl::RGResourceID resource_id;
			PassNodeID creator;
			PassNodeID writer;
			ResourceNodeID write_target_node;
			Vector<PassNodeID> readers;

			ResourceNode(RGResourceType resource_type, impl::RGResourceID resource_id) : resource_type(resource_type), resource_id(resource_id) {}
			ResourceNode(RGResourceType resource_type, impl::RGResourceID resource_id, PassNodeID creator) :
		        resource_type(resource_type), resource_id(resource_id), creator(creator) {}
		};

	}

    template <PipelineFlags pipeline_flags>
    RGDependencyBuilder<pipeline_flags>::RGDependencyBuilder(const PassNodeID pass_id, PassBaseNode& pass_node,
        RenderGraph& render_graph) : pass_id_(pass_id), pass_node_(pass_node), render_graph_(render_graph)
    {
    }

	template<PipelineFlags pipeline_flags>
	inline BufferNodeID RGDependencyBuilder<pipeline_flags>::add_shader_buffer(BufferNodeID node_id, ShaderStageFlags stage_flags, ShaderBufferReadUsage usage_type)
	{
		static_assert(pipeline_flags.test_any({PipelineType::RASTER, PipelineType::COMPUTE }));
		render_graph_.read_resource_node(node_id, pass_id_);
		pass_node_.shader_buffer_read_accesses_.push_back({ node_id, stage_flags, usage_type });
		return node_id;
	}

	template<PipelineFlags pipeline_flags>
	inline BufferNodeID RGDependencyBuilder<pipeline_flags>::add_shader_buffer(BufferNodeID node_id, ShaderStageFlags stage_flags, ShaderBufferWriteUsage usage_type)
	{
		static_assert(pipeline_flags.test_any({PipelineType::RASTER, PipelineType::COMPUTE}));
		const auto out_node_id = render_graph_.write_resource_node(node_id, pass_id_);
		pass_node_.shader_buffer_write_accesses_.push_back({ node_id, out_node_id, stage_flags, usage_type });
		return out_node_id;
	}

	template<PipelineFlags pipeline_flags>
	inline TextureNodeID RGDependencyBuilder<pipeline_flags>::add_shader_texture(TextureNodeID node_id, ShaderStageFlags stage_flags, ShaderTextureReadUsage usage_type, SubresourceIndexRange view)
	{
		static_assert(pipeline_flags.test_any({PipelineType::RASTER, PipelineType::COMPUTE}));
		render_graph_.read_resource_node(node_id, pass_id_);
		pass_node_.shader_texture_read_accesses_.push_back({ node_id, stage_flags, usage_type, view });
		return node_id;
	}

	template<PipelineFlags pipeline_flags>
	inline TextureNodeID RGDependencyBuilder<pipeline_flags>::add_shader_texture(TextureNodeID node_id, ShaderStageFlags stage_flags, ShaderTextureWriteUsage usage_type, SubresourceIndexRange view)
	{
		static_assert(pipeline_flags.test_any({PipelineType::RASTER,PipelineType::COMPUTE}));
		const auto out_node_id = render_graph_.write_resource_node(node_id, pass_id_);
		pass_node_.shader_texture_write_accesses_.push_back({ node_id, out_node_id, stage_flags, usage_type, view });
		return out_node_id;
	}

	template<PipelineFlags pipeline_flags>
	inline BufferNodeID RGDependencyBuilder<pipeline_flags>::add_vertex_buffer(BufferNodeID node_id)
	{
		static_assert(pipeline_flags.test(PipelineType::RASTER));
		render_graph_.read_resource_node(node_id, pass_id_);
		pass_node_.vertex_buffers_.push_back(node_id);
		return node_id;
	}

	template<PipelineFlags pipeline_flags>
	inline BufferNodeID RGDependencyBuilder<pipeline_flags>::add_index_buffer(BufferNodeID node_id)
	{
		static_assert(pipeline_flags.test(PipelineType::RASTER));
		render_graph_.read_resource_node(node_id, pass_id_);
		pass_node_.index_buffers_.push_back(node_id);
		return node_id;
	}

    template <PipelineFlags pipeline_flags>
    BufferNodeID RGDependencyBuilder<pipeline_flags>::add_src_buffer(BufferNodeID node_id)
    {
		static_assert(pipeline_flags.test(PipelineType::NON_SHADER));
		render_graph_.read_resource_node(node_id, pass_id_);
		pass_node_.source_buffers_.add({ node_id });
		return node_id;
    }

    template <PipelineFlags pipeline_flags>
    BufferNodeID RGDependencyBuilder<pipeline_flags>::add_dst_buffer(BufferNodeID node_id, TransferDataSource data_source)
    {
		static_assert(pipeline_flags.test(PipelineType::NON_SHADER));
		BufferNodeID out_node_id = render_graph_.write_resource_node(node_id, pass_id_);
		pass_node_.destination_buffers_.add({ .data_source = data_source, .input_node_id = node_id, .output_node_id = out_node_id });
		return out_node_id;
    }

    template <PipelineFlags pipeline_flags>
    TextureNodeID RGDependencyBuilder<pipeline_flags>::add_src_texture(TextureNodeID node_id)
    {
		static_assert(pipeline_flags.test(PipelineType::NON_SHADER));
		render_graph_.read_resource_node(node_id, pass_id_);
		pass_node_.source_textures_.add({ node_id });
		return node_id;
    }

    template <PipelineFlags pipeline_flags>
    TextureNodeID RGDependencyBuilder<pipeline_flags>::add_dst_texture(TextureNodeID node_id,
        TransferDataSource data_source)
    {
		static_assert(pipeline_flags.test(PipelineType::NON_SHADER));
		TextureNodeID out_node_id = render_graph_.write_resource_node(node_id, pass_id_);
		pass_node_.destination_textures_.add({ .data_source = data_source, .input_node_id = node_id, .output_node_id = out_node_id });
		return out_node_id;
    }

    template <PipelineFlags pipeline_flags>
    void RGDependencyBuilder<pipeline_flags>::set_render_target(const RGRenderTargetDesc& render_target_desc)
    {
		static_assert(pipeline_flags.test(PipelineType::RASTER));
		auto to_output_attachment = [this]<typename AttachmentDesc>(const AttachmentDesc attachment_desc) -> auto
		{
			AttachmentAccess<AttachmentDesc> access = {
				.out_node_id = render_graph_.write_resource_node(attachment_desc.node_id, pass_id_),
				.desc = attachment_desc
			};
			return access;
		};

		std::ranges::transform(render_target_desc.color_attachments,
			std::back_inserter(pass_node_.render_target_.color_attachments), to_output_attachment);
		std::ranges::transform(render_target_desc.resolve_attachments,
			std::back_inserter(pass_node_.render_target_.resolve_attachments), to_output_attachment);
		if (render_target_desc.depth_stencil_attachment.node_id.is_valid())
		{
			const auto& depth_desc = render_target_desc.depth_stencil_attachment;
			const TextureNodeID out_node_id = depth_desc.depth_write_enable ? 
				depth_desc.node_id : render_graph_.write_resource_node(depth_desc.node_id, pass_id_);
			pass_node_.render_target_.depth_stencil_attachment = {
				.out_node_id = out_node_id,
				.desc = depth_desc
			};
		}
		pass_node_.render_target_.dimension = render_target_desc.dimension;
		pass_node_.render_target_.sample_count = render_target_desc.sample_count;
    }

    template <PipelineFlags pipeline_flags, typename Parameter, typename Execute>
    void PassNode<pipeline_flags, Parameter, Execute>::execute(RenderGraphRegistry& registry, 
		impl::RenderCompiler& render_compiler,
        const VkRenderPassBeginInfo* render_pass_begin_info, impl::CommandPools& command_pools, System& gpu_system) const
    {
		CommandList<pipeline_flags> command_list(render_compiler, render_pass_begin_info, command_pools, gpu_system);
		execute_(parameter_, registry, command_list);
    }
}
