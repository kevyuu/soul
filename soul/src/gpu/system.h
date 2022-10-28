#pragma once

#include "core/cstring.h"
#include "gpu/type.h"
#include "gpu/intern/bindless_descriptor_allocator.h"

#if defined(SOUL_ASSERT_ENABLE)
#define SOUL_VK_CHECK(result, message, ...) SOUL_ASSERT(0, result == VK_SUCCESS, "result = %d | " message, result, ##__VA_ARGS__)
#else
#include <cstdlib>
#define SOUL_VK_CHECK(expr, message, ...) do { VkResult _result = expr; if (_result != VK_SUCCESS) { SOUL_LOG_ERROR("Vulkan error| expr = %s, result = %s ", #expr, _result); SOUL_LOG_ERROR("Message = %s", ##__VA_ARGS__); } } while(0)
#endif

#include <span>

namespace soul::gpu::impl
{
	class RenderCompiler;
	class RenderGraphExecution;
}

namespace soul::gpu
{
	class RenderGraph;

	class System {
	public:
		explicit System(memory::Allocator* allocator) : _db(allocator) {}

		struct Config {
			WSI* wsi = nullptr;
			uint16 max_frame_in_flight = 0;
			uint16 thread_count = 0;
			soul_size transient_pool_size = 16 * ONE_MEGABYTE;
		};

		void init(const Config& config);
		void init_frame_context(const System::Config& config);

		const GPUProperties& get_gpu_properties() const;

		void shutdown();

		BufferID create_buffer(const BufferDesc& desc);
		BufferID create_buffer(const BufferDesc& desc, const void* data);
		BufferID create_transient_buffer(const BufferDesc& desc);
		void flush_buffer(BufferID buffer_id);
		void destroy_buffer_descriptor(BufferID buffer_id);
		void destroy_buffer(BufferID buffer_id);
		impl::Buffer& get_buffer(BufferID buffer_id);
		const impl::Buffer& get_buffer(BufferID buffer_id) const;
		GPUAddress get_gpu_address(BufferID buffer_id) const;

		TextureID create_texture(const TextureDesc& desc);
		TextureID create_texture(const TextureDesc& desc, const TextureLoadDesc& load_desc);
		TextureID create_texture(const TextureDesc& desc, ClearValue clear_value);
		void flush_texture(TextureID texture_id, TextureUsageFlags usage_flags);
		uint32 get_texture_mip_levels(TextureID texture_id) const;
		const TextureDesc& get_texture_desc(TextureID texture_id) const;
		void destroy_texture_descriptor(TextureID texture_id);
		void destroy_texture(TextureID texture_id);
		impl::Texture& get_texture(TextureID texture_id);
		const impl::Texture& get_texture(TextureID texture_id) const;
		impl::TextureView get_texture_view(TextureID texture_id, uint32 level, uint32 layer = 0);
		impl::TextureView get_texture_view(TextureID texture_id, SubresourceIndex subresource_index);
		impl::TextureView get_texture_view(TextureID texture_id, std::optional<SubresourceIndex> subresource);

		soul_size get_blas_size_requirement(const BlasBuildDesc& build_desc);
		BlasID create_blas(const BlasDesc& desc, BlasGroupID blas_group_id = BlasGroupID::null());
		void destroy_blas(BlasID blas_id);
		const impl::Blas& get_blas(BlasID blas_id) const;
		impl::Blas& get_blas(BlasID blas_id);
		GPUAddress get_gpu_address(BlasID blas_id) const;

		BlasGroupID create_blas_group(const char* name);
		void destroy_blas_group(BlasGroupID blas_group_id);
		const impl::BlasGroup& get_blas_group(BlasGroupID blas_group_id) const;
		impl::BlasGroup& get_blas_group(BlasGroupID blas_group_id);

		soul_size get_tlas_size_requirement(const TlasBuildDesc& build_desc);
		TlasID create_tlas(const TlasDesc& desc);
		void destroy_tlas(TlasID tlas_id);
		const impl::Tlas& get_tlas(TlasID tlas_id) const;
		impl::Tlas& get_tlas(TlasID tlas_id);

	    expected<ProgramID, Error> create_program(const ProgramDesc& program_desc);
	    impl::Program* get_program_ptr(ProgramID program_id);
		const impl::Program& get_program(ProgramID program_id);
		
		ShaderTableID create_shader_table(const ShaderTableDesc& shader_table_desc);
		void destroy_shader_table(ShaderTableID shader_table_id);
		const impl::ShaderTable& get_shader_table(ShaderTableID shader_table_id) const;
		impl::ShaderTable& get_shader_table(ShaderTableID shader_table_id);

		auto request_pipeline_state(const GraphicPipelineStateDesc& key, VkRenderPass render_pass, TextureSampleCount sample_count) -> PipelineStateID;
		PipelineStateID request_pipeline_state(const ComputePipelineStateDesc& key);
		const impl::PipelineState& get_pipeline_state(PipelineStateID pipeline_state_id);
		VkPipelineLayout get_bindless_pipeline_layout() const;
		impl::BindlessDescriptorSets get_bindless_descriptor_sets() const;

		SamplerID request_sampler(const SamplerDesc& desc);

		DescriptorID get_ssbo_descriptor_id(BufferID buffer_id) const;
		DescriptorID get_srv_descriptor_id(TextureID texture_id, std::optional<SubresourceIndex> subresource_index = std::nullopt);
		DescriptorID get_uav_descriptor_id(TextureID texture_id, std::optional<SubresourceIndex> subresource_index = std::nullopt);
		DescriptorID get_sampler_descriptor_id(SamplerID sampler_id) const;
		DescriptorID get_as_descriptor_id(TlasID tlas_id) const;

		impl::BinarySemaphore create_binary_semaphore();
		void destroy_binary_semaphore(impl::BinarySemaphore id);
		
		VkEvent create_event();
		void destroy_event(VkEvent event);

		void execute(const RenderGraph& render_graph);

		void flush();
		void flush_frame();
		void begin_frame();
		void end_frame();

		void recreate_swapchain();
		vec2ui32 get_swapchain_extent();
		TextureID get_swapchain_texture();

		impl::FrameContext& get_frame_context();

		VkRenderPass request_render_pass(const impl::RenderPassKey& key);

		VkFramebuffer create_framebuffer(const VkFramebufferCreateInfo& info);
		void destroy_framebuffer(VkFramebuffer framebuffer);

		impl::QueueData get_queue_data_from_queue_flags(QueueFlags flags) const;

		impl::Database _db;

		friend class impl::RenderCompiler;
		friend class impl::RenderGraphExecution;

	private:

		bool is_owned_by_presentation_engine(TextureID texture_id);
		BufferID create_buffer(const BufferDesc& desc, bool use_linear_pool);
		BufferID create_staging_buffer(soul_size size);
		VmaAllocator get_gpu_allocator();

		VkResult acquire_swapchain();

		void wait_sync_counter(impl::TimelineSemaphore sync_counter);

		void calculate_gpu_properties();

		VkAccelerationStructureBuildSizesInfoKHR get_as_build_size_info(const TlasBuildDesc& build_desc);
		VkAccelerationStructureBuildSizesInfoKHR get_as_build_size_info(const BlasBuildDesc& build_desc);
		VkAccelerationStructureBuildSizesInfoKHR get_as_build_size_info(const VkAccelerationStructureBuildGeometryInfoKHR& build_info, const uint32* max_primitives_counts);

		void add_to_blas_group(BlasID blas_id, BlasGroupID blas_group_id);
		void remove_from_blas_group(BlasID blas_id);

		Config config_;
	};
}
