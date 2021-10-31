#pragma once

#include "core/type_traits.h"
#include "gpu/data.h"
#include "gpu/render_graph.h"

#if defined(SOUL_ASSERT_ENABLE)
#define SOUL_VK_CHECK(result, message, ...) SOUL_ASSERT(0, result == VK_SUCCESS, "result = %d | " message, result, ##__VA_ARGS__)
#else
#define SOUL_VK_CHECK(result, message, ...) result
#endif

namespace Soul::GPU
{

	class GraphicBaseNode;

	class System {
	public:
		explicit System(Memory::Allocator* allocator) : _db(allocator) {}

		struct Config {
			void* windowHandle = nullptr;
			uint32 swapchainWidth = 0;
			uint32 swapchainHeight = 0;
			uint16 maxFrameInFlight = 0;
			uint16 threadCount = 0;
		};

		void init(const Config& config);
		void _frameContextInit(const System::Config& config);

		void shutdown();

		BufferID bufferCreate(const BufferDesc& desc);
		BufferID bufferCreate(const BufferDesc& desc, const void* data);
		void bufferDestroy(BufferID bufferID);
		impl::Buffer* _bufferPtr(BufferID bufferID);
		const impl::Buffer& _bufferRef(BufferID bufferID);

		TextureID textureCreate(const TextureDesc& desc);
		TextureID textureCreate(const TextureDesc& desc, const void* data, uint32 dataSize);
		TextureID textureCreate(const TextureDesc& desc, ClearValue clearValue);

		TextureID _textureExportCreate(TextureID srcTextureID);
		void textureDestroy(TextureID textureID);
		impl::Texture* _texturePtr(TextureID textureID);
		const impl::Texture& _textureRef(TextureID textureID);
		VkImageView _textureGetMipView(TextureID textureID, uint32 level);

		ShaderID shaderCreate(const ShaderDesc& desc, ShaderStage stage);
		void shaderDestroy(ShaderID shaderID);
		impl::Shader* _shaderPtr(ShaderID shaderID);

		VkDescriptorSetLayout _descriptorSetLayoutRequest(const impl::DescriptorSetLayoutKey& key);

		ProgramID programRequest(const ProgramDesc& key);
		impl::Program* _programPtr(ProgramID programID);
		const impl::Program& _programRef(ProgramID programID);

		VkPipeline _pipelineCreate(const ComputeBaseNode& node, ProgramID programID);

		PipelineStateID _pipelineStateRequest(const PipelineStateDesc& key, VkRenderPass renderPass);
		impl::PipelineState* _pipelineStatePtr(PipelineStateID pipelineStateID);
		const impl::PipelineState& _pipelineStateRef(PipelineStateID pipelineStateID);

		SamplerID samplerRequest(const SamplerDesc& desc);

		ShaderArgSetID _shaderArgSetRequest(const ShaderArgSetDesc& desc);
		const impl::ShaderArgSet& _shaderArgSetRef(ShaderArgSetID argSetID);

		SemaphoreID _semaphoreCreate();
		void _semaphoreReset(SemaphoreID ID);
		void _semaphoreDestroy(SemaphoreID id);
		impl::Semaphore* _semaphorePtr(SemaphoreID id);

		VkEvent _eventCreate();
		void _eventDestroy(VkEvent event);

		void renderGraphExecute(const RenderGraph& renderGraph);

		void frameFlush();
		void _frameBegin();
		void _frameEnd();

		Vec2ui32 getSwapchainExtent();
		TextureID getSwapchainTexture();

		void _surfaceCreate(void* windowHandle, VkSurfaceKHR* surface);

		impl::_FrameContext& _frameContext();

		VkRenderPass _renderPassRequest(const impl::RenderPassKey& key);
		VkRenderPass _renderPassCreate(const VkRenderPassCreateInfo& info);
		void _renderPassDestroy(VkRenderPass renderPass);

		VkFramebuffer _framebufferCreate(const VkFramebufferCreateInfo& info);
		void _framebufferDestroy(VkFramebuffer framebuffer);

		impl::QueueData _getQueueDataFromQueueFlags(QueueFlags flags);

		impl::Database _db;
	};

}
