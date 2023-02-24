//
// Created by Kevin Yudi Utama on 26/12/19.
//

#pragma once
#include "gpu/type.h"
#include "memory/allocator.h"

namespace soul::gpu::impl
{

	struct BufferBarrier {
		PipelineStageFlags stage_flags;
		AccessFlags access_flags;
		uint32 buffer_info_idx = 0;
	};

	struct TextureBarrier {
		PipelineStageFlags stage_flags;
		AccessFlags access_flags;

		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		uint32 texture_info_idx = 0;
		SubresourceIndex view;
	};

	struct BufferExecInfo {
		PassNodeID first_pass;
		PassNodeID last_pass;
		BufferUsageFlags usage_flags;
		QueueFlags queue_flags;
		BufferID buffer_id;

		VkEvent pending_event = VK_NULL_HANDLE;
		Semaphore pending_semaphore = TimelineSemaphore::null();
		ResourceCacheState cache_state;

		Vector<PassNodeID> passes;
		uint32 pass_counter = 0;
	};

	struct TextureViewExecInfo {
		VkEvent pending_event = VK_NULL_HANDLE;
		Semaphore pending_semaphore = TimelineSemaphore::null();
		ResourceCacheState cache_state;
		Vector<PassNodeID> passes;
		uint32 pass_counter = 0;
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	};

	struct TextureExecInfo {
		PassNodeID first_pass;
		PassNodeID last_pass;
		TextureUsageFlags usage_flags;
		QueueFlags queue_flags;
		TextureID texture_id;
		TextureViewExecInfo* view = nullptr;
		uint32 mip_levels = 0;
		uint32 layers = 0;

		[[nodiscard]] soul_size get_view_count() const
		{
			return soul::cast<soul_size>(mip_levels) * layers;
		}

		[[nodiscard]] soul_size get_view_index(const SubresourceIndex index) const
		{
			return index.get_layer() * mip_levels + index.get_level();
		}

		[[nodiscard]] TextureViewExecInfo* get_view(const SubresourceIndex index) const
		{
			return view + get_view_index(index);
		}
	};

	

	class PassDependencyGraph
	{
	public:
		enum class DependencyType : uint8
		{
			READ_AFTER_WRITE,
            WRITE_AFTER_WRITE,
			WRITE_AFTER_READ,
			COUNT
		};
		using DependencyFlags = FlagSet<DependencyType>;
		static constexpr DependencyFlags OP_AFTER_WRITE_DEPENDENCY = 
		    { DependencyType::READ_AFTER_WRITE, DependencyType::WRITE_AFTER_WRITE };

		using NodeList = Vector<PassNodeID>;
		using NodeDependencyMatrix = FlagMap<DependencyType, BitVector<>>;

		PassDependencyGraph(soul_size pass_node_count, std::span<const impl::ResourceNode> resource_nodes);
		[[nodiscard]] DependencyFlags get_dependency_flags(PassNodeID src_node_id, PassNodeID dst_node_id) const;
		[[nodiscard]] std::span<const PassNodeID> get_dependencies( const PassNodeID node_id) const { return dependencies_[node_id.id]; }
		[[nodiscard]] std::span<const PassNodeID> get_dependants(const PassNodeID node_id) const { return dependants_[node_id.id]; }
		[[nodiscard]] soul_size get_dependency_level(PassNodeID node_id) const { return dependency_levels_[node_id.id]; }
		void set_dependency(PassNodeID src_node_id, PassNodeID dst_node_id, DependencyType dependency_type);

	private:
		soul_size pass_node_count_;
		NodeDependencyMatrix dependency_matrixes_;
		Vector<NodeList> dependencies_;
		Vector<NodeList> dependants_;
		Vector<soul_size> dependency_levels_;


		static constexpr auto UNINITIALIZED_DEPENDENCY_LEVEL = ~0u;
		[[nodiscard]] soul_size get_pass_node_count() const;
		[[nodiscard]] soul_size get_dependency_matrix_index(PassNodeID src_node_id, PassNodeID dst_node_id) const;

		soul_size calculate_dependency_level(PassNodeID pass_node_id);
	};

	struct PassExecInfo {
		PassBaseNode* pass_node = nullptr;
		Vector<BufferBarrier> buffer_flushes;
		Vector<BufferBarrier> buffer_invalidates;
		Vector<TextureBarrier> texture_flushes;
		Vector<TextureBarrier> texture_invalidates;
	};

	struct PhysicalEvent
	{
		VkEvent event;
		VkDependencyInfo dependency_info;
	};

	struct PassSynchronizations
	{
		Vector<PassNodeID> dependency_events;
		Vector<VkDependencyInfo> pipeline_barriers;

	};

	class RenderGraphExecution {
	public:
		RenderGraphExecution(const RenderGraph* render_graph, System* system, memory::Allocator* allocator, 
			CommandQueues& command_queues, CommandPools& command_pools):
			render_graph_(render_graph), gpu_system_(system),
            command_queues_(command_queues), command_pools_(command_pools), buffer_infos_(allocator),
            texture_infos_(allocator), pass_infos_(allocator),
		    pass_dependency_graph_(render_graph->get_pass_nodes().size(), render_graph->get_resource_nodes())
        {}

		RenderGraphExecution() = delete;
		RenderGraphExecution(const RenderGraphExecution& other) = delete;
		RenderGraphExecution& operator=(const RenderGraphExecution& other) = delete;

		RenderGraphExecution(RenderGraphExecution&& other) = delete;
		RenderGraphExecution& operator=(RenderGraphExecution&& other) = delete;

		~RenderGraphExecution() = default;

		void init();
		void run();
		void cleanup();

		[[nodiscard]] bool is_external(const BufferExecInfo& info) const;
		[[nodiscard]] bool is_external(const TextureExecInfo& info) const;
		[[nodiscard]] BufferID get_buffer_id(BufferNodeID node_id) const;
		[[nodiscard]] TextureID get_texture_id(TextureNodeID node_id) const;
		[[nodiscard]] Buffer& get_buffer(BufferNodeID node_id) const;
		[[nodiscard]] Texture& get_texture(TextureNodeID node_id) const;
		[[nodiscard]] uint32 get_buffer_info_index(BufferNodeID node_id) const;
		[[nodiscard]] uint32 get_texture_info_index(TextureNodeID nodeID) const;

	private:
		const RenderGraph* render_graph_;
		System* gpu_system_;

		FlagMap<QueueType, VkEvent> external_events_;
		FlagMap<QueueType, PipelineStageFlags> external_events_stage_flags_;
		CommandQueues& command_queues_;
		CommandPools& command_pools_;

		Vector<BufferExecInfo> buffer_infos_;
		Slice<BufferExecInfo> internal_buffer_infos_;
		Slice<BufferExecInfo> external_buffer_infos_;

		Vector<TextureExecInfo> texture_infos_;
		Slice<TextureExecInfo> internal_texture_infos_;
		Slice<TextureExecInfo> external_texture_infos_;
		Vector<TextureViewExecInfo> texture_view_infos_;

		Vector<PassExecInfo> pass_infos_;
		
		PassDependencyGraph pass_dependency_graph_;
		BitVector<> active_passes_;
		Vector<PassNodeID> pass_order_;

		void compute_active_passes();
		void compute_pass_order();

		[[nodiscard]] VkRenderPass create_render_pass(uint32 pass_index);
		[[nodiscard]] VkFramebuffer create_framebuffer(uint32 pass_index, VkRenderPass render_pass);
		void sync_external();
		void execute_pass(uint32 pass_index, PrimaryCommandBuffer command_buffer);

		void init_shader_buffers(std::span<const ShaderBufferReadAccess> access_list, PassNodeID pass_node_id, QueueType queue_type);
		void init_shader_buffers(std::span<const ShaderBufferWriteAccess> access_list, PassNodeID pass_node_id, QueueType queue_type);
		void init_shader_textures(std::span<const ShaderTextureReadAccess> access_list, PassNodeID pass_node_id, QueueType queue_type);
		void init_shader_textures(std::span<const ShaderTextureWriteAccess> access_list, PassNodeID pass_node_id, QueueType queue_type);

	};
}
