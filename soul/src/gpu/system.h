#pragma once

#include "core/type_traits.h"
#include "gpu/type.h"
#include "gpu/render_graph.h"
#if defined(SOUL_ASSERT_ENABLE)
#define SOUL_VK_CHECK(result, message, ...) SOUL_ASSERT(0, result == VK_SUCCESS, "result = %d | " message, result, ##__VA_ARGS__)
#else
#include <stdlib.h>
#define SOUL_VK_CHECK(result, message, ...) do { if (result != VK_SUCCESS) { SOUL_LOG_ERROR("Vulkan Error = " #result ", result = %d | " message, result, ##__VA_ARGS__); exit(0); } } while(0)
#endif

namespace soul::gpu
{

	class GraphicBaseNode;

	class System {
	public:
		explicit System(memory::Allocator* allocator) : _db(allocator) {}

		struct Config {
			void* windowHandle = nullptr;
			uint32 swapchainWidth = 0;
			uint32 swapchainHeight = 0;
			uint16 maxFrameInFlight = 0;
			uint16 threadCount = 0;
		};

		void init(const Config& config);
		void init_frame_context(const System::Config& config);

		void shutdown();

		BufferID create_buffer(const BufferDesc& desc);
		BufferID create_buffer(const BufferDesc& desc, const void* data);
		void finalize_buffer(BufferID buffer_id);
		void destroy_buffer(BufferID bufferID);
		impl::Buffer* get_buffer_ptr(BufferID buffer_id);
		const impl::Buffer& get_buffer(BufferID buffer_id) const;

		TextureID create_texture(const TextureDesc& desc);
		TextureID create_texture(const TextureDesc& desc, const TextureLoadDesc& load_desc);
		TextureID create_texture(const TextureDesc& desc, ClearValue clear_value);
		void finalize_texture(TextureID texture_id, TextureUsageFlags usage_flags);
		uint32 get_texture_mip_levels(TextureID texture_id) const;
		const TextureDesc& get_texture_desc(const TextureID texture_id) const;
		
		void destroy_texture(TextureID textureID);
		impl::Texture* get_texture_ptr(TextureID texture_id);
		const impl::Texture& get_texture(TextureID texture_id) const;
		VkImageView get_texture_view(TextureID texture_id, uint32 level, uint32 layer = 0);
		VkImageView get_texture_view(TextureID texture_id, SubresourceIndex subresource_index);
		VkImageView get_texture_view(TextureID texture_id, const std::optional<SubresourceIndex> subresource);

		ShaderID create_shader(const ShaderDesc& desc, ShaderStage stage);
		void destroy_shader(ShaderID shader_id);
		impl::Shader* get_shader_ptr(ShaderID shader_id);

		VkDescriptorSetLayout request_descriptor_layout(const impl::DescriptorSetLayoutKey& key);

		ProgramID request_program(const ProgramDesc& key);
		impl::Program* get_program_ptr(ProgramID program_id);
		const impl::Program& get_program(ProgramID program_id);


		PipelineStateID request_pipeline_state(const GraphicPipelineStateDesc& key, VkRenderPass renderPass, const TextureSampleCount sample_count);
		PipelineStateID request_pipeline_state(const ComputePipelineStateDesc& key);
		impl::PipelineState* get_pipeline_state_ptr(PipelineStateID pipeline_state_id);
		const impl::PipelineState& get_pipeline_state(PipelineStateID pipeline_state_id);

		SamplerID request_sampler(const SamplerDesc& desc);

		ShaderArgSetID request_shader_arg_set(const ShaderArgSetDesc& desc);
		impl::ShaderArgSet get_shader_arg_set(ShaderArgSetID arg_set_id);

		SemaphoreID create_semaphore();
		void reset_semaphore(SemaphoreID ID);
		void destroy_semaphore(SemaphoreID id);
		impl::Semaphore* get_semaphore_ptr(SemaphoreID id);

		VkEvent create_event();
		void destroy_event(VkEvent event);

		void execute(const RenderGraph& renderGraph);

		void frameFlush();
		void _frameBegin();
		void _frameEnd();

		Vec2ui32 getSwapchainExtent();
		TextureID getSwapchainTexture();

		void create_surface(void* windowHandle, VkSurfaceKHR* surface);

		impl::_FrameContext& get_frame_context();

		VkRenderPass request_render_pass(const impl::RenderPassKey& key);

		VkFramebuffer create_framebuffer(const VkFramebufferCreateInfo& info);
		void destroy_framebuffer(VkFramebuffer framebuffer);

		impl::QueueData get_queue_data_from_queue_flags(QueueFlags flags) const;

		impl::Database _db;
	};

}
