#include "core/util.h"

#include "runtime/runtime.h"
#include "runtime/scope_allocator.h"

#include "core/math.h"

#include "gpu/system.h"
#include "gpu/render_graph_registry.h"
#include "gpu/intern/render_graph_execution.h"
#include "gpu/intern/enum_mapping.h"
#include "gpu/intern/render_compiler.h"

#include <chrono>

#include <volk/volk.h>

namespace Soul::GPU::impl
{

	static constexpr EnumArray<PassType, QueueType> PASS_TYPE_TO_QUEUE_TYPE_MAP({
		QueueType::COUNT,
		QueueType::GRAPHIC,
		QueueType::COMPUTE,
		QueueType::TRANSFER,
	});

	static constexpr EnumArray<ResourceOwner, PassType> RESOURCE_OWNER_TO_PASS_TYPE_MAP({
		PassType::NONE,
		PassType::GRAPHIC,
		PassType::COMPUTE,
		PassType::TRANSFER,
		PassType::NONE
	});

	static auto getBufferUsageFlagsFromDescriptorType = [](DescriptorType type) -> BufferUsageFlags {
		SOUL_ASSERT(0, DescriptorTypeUtil::IsBuffer(type), "");
		static constexpr BufferUsageFlags mapping[] = {
			0,
			BUFFER_USAGE_UNIFORM_BIT,
			0,
			BUFFER_USAGE_UNIFORM_BIT,
			0,
		};
		static_assert(SOUL_ARRAY_LEN(mapping) == uint64(DescriptorType::COUNT) , "");
		return mapping[uint64(type)];
	};

	static auto getTextureUsageFlagsFromDescriptorType = [](DescriptorType type) -> TextureUsageFlags {
		SOUL_ASSERT(0, DescriptorTypeUtil::IsTexture(type), "");
		static constexpr TextureUsageFlags mapping[] = {
			0,
			0,
			TEXTURE_USAGE_SAMPLED_BIT,
			0,
			TEXTURE_USAGE_STORAGE_BIT
		};
		static_assert(SOUL_ARRAY_LEN(mapping) == uint64(DescriptorType::COUNT), "");
		return mapping[uint64(type)];
	};

	static auto updateBufferInfo = [](BufferExecInfo* bufferInfo, QueueFlagBits queueFlag,
	                                  VkFlags usageFlags, PassNodeID passID) {

		bufferInfo->usageFlags |= usageFlags;
		bufferInfo->queueFlags |= queueFlag;
		if (bufferInfo->firstPass == PASS_NODE_ID_NULL) {
			bufferInfo->firstPass = passID;
		}
		bufferInfo->lastPass = passID;
		bufferInfo->passes.add(passID);
	};

	static auto updateTextureInfo = [](TextureExecInfo* textureInfo, QueueFlagBits queueFlag,
	                                   TextureUsageFlags usageFlags, PassNodeID passID) {

		textureInfo->usageFlags |= usageFlags;
		textureInfo->queueFlags |= queueFlag;
		if (textureInfo->firstPass == PASS_NODE_ID_NULL) {
			textureInfo->firstPass = passID;
		}
		textureInfo->lastPass = passID;
		textureInfo->passes.add(passID);
	};

	static auto getBufferUsageFromShaderReadUsage = [](ShaderBufferReadUsage shaderUsage) -> BufferUsageFlags {
		static constexpr BufferUsageFlags mapping[] = {
			BUFFER_USAGE_UNIFORM_BIT,
			BUFFER_USAGE_STORAGE_BIT,
		};
		static_assert(SOUL_ARRAY_LEN(mapping) == uint64(ShaderBufferReadUsage::COUNT), "");
		return mapping[uint64(shaderUsage)];
	};

	static auto getBufferUsageFromShaderWriteUsage = [](ShaderBufferWriteUsage shaderUsage) -> BufferUsageFlags {
		static constexpr BufferUsageFlags mapping[] = {
			BUFFER_USAGE_STORAGE_BIT
		};
		static_assert(SOUL_ARRAY_LEN(mapping) == uint64(ShaderBufferWriteUsage::COUNT), "");
		return mapping[uint64(shaderUsage)];
	};

	static auto getTextureUsageFromShaderReadUsage = [](ShaderTextureReadUsage shaderUsage) -> TextureUsageFlags {
		static constexpr TextureUsageFlags mapping[] = {
			TEXTURE_USAGE_SAMPLED_BIT,
			TEXTURE_USAGE_STORAGE_BIT
		};
		static_assert(SOUL_ARRAY_LEN(mapping) == uint64(ShaderTextureReadUsage::COUNT), "");
		return mapping[uint64(shaderUsage)];
	};

	static auto getTextureUsageFromShaderWriteUsage = [](ShaderTextureWriteUsage shaderUsage) -> TextureUsageFlags {
		static constexpr TextureUsageFlags mapping[] = {
			TEXTURE_USAGE_STORAGE_BIT
		};
		static_assert(SOUL_ARRAY_LEN(mapping) == uint64(ShaderTextureWriteUsage::COUNT), "");
		return mapping[uint64(shaderUsage)];
	};

	void RenderGraphExecution::init() {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE_WITH_NAME("Render Graph Execution Init");
		passInfos.resize(_renderGraph->_passNodes.size());

		bufferInfos.resize(_renderGraph->_internalBuffers.size() + _renderGraph->_externalBuffers.size());
		internalBufferInfos.set(&bufferInfos, 0, _renderGraph->_internalBuffers.size());
		externalBufferInfos.set(&bufferInfos, _renderGraph->_internalBuffers.size(), bufferInfos.size());

		textureInfos.resize(_renderGraph->_internalTextures.size() + _renderGraph->_externalTextures.size());
		internalTextureInfos.set(&textureInfos, 0, _renderGraph->_internalTextures.size());
		externalTextureInfos.set(&textureInfos, _renderGraph->_internalTextures.size(), textureInfos.size());

		for (int i = 0; i < passInfos.size(); i++) {
			PassNodeID passNodeID = PassNodeID(i);
			const PassNode& passNode = *_renderGraph->_passNodes[i];
			PassExecInfo& passInfo = passInfos[i];

			switch (passNode.type) {
			case PassType::NONE: {
				break;
			}
			case PassType::COMPUTE: {
				/*auto computeNode = (const ComputeBaseNode *) &passNode;
				    ProgramDesc programKey;
				    programKey.shaderIDs[ShaderStage::COMPUTE] = computeNode->pipelineConfig.computeShaderID;
				    passInfo.programID = _gpuSystem->programRequest(programKey);
                    _initInShaderBuffers(computeNode->shaderBufferReadAccesses, i, QUEUE_COMPUTE_BIT);
                    _initOutShaderBuffers(computeNode->shaderBufferWriteAccesses, i, QUEUE_COMPUTE_BIT);
                    _initShaderTextures(computeNode->shaderTextureReadAccesses, i, QUEUE_COMPUTE_BIT);
                    _initShaderTextures(computeNode->shaderTextureWriteAccesses, i, QUEUE_COMPUTE_BIT);*/
				break;
			}
			case PassType::GRAPHIC: {

				auto graphicNode = (const GraphicBaseNode *) &passNode;
				for (const BufferNodeID nodeID : graphicNode->vertexBuffers) {
					SOUL_ASSERT(0, nodeID != BUFFER_NODE_ID_NULL, "");

					uint32 bufferInfoID = getBufferInfoIndex(nodeID);

					passInfo.bufferInvalidates.add(BufferBarrier());
					BufferBarrier& invalidateBarrier = passInfo.bufferInvalidates.back();
					invalidateBarrier.stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
					invalidateBarrier.accessFlags = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
					invalidateBarrier.bufferInfoIdx = bufferInfoID;


					passInfo.bufferFlushes.add(BufferBarrier());
					BufferBarrier& flushBarrier = passInfo.bufferFlushes.back();
					flushBarrier.stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
					flushBarrier.accessFlags = 0;
					flushBarrier.bufferInfoIdx = bufferInfoID;

					updateBufferInfo(&bufferInfos[bufferInfoID], QUEUE_GRAPHIC_BIT, BUFFER_USAGE_VERTEX_BIT, PassNodeID(i));
				}

				for (const BufferNodeID nodeID : graphicNode->indexBuffers) {
					SOUL_ASSERT(0, nodeID != BUFFER_NODE_ID_NULL, "");

					uint32 bufferInfoID = getBufferInfoIndex(nodeID);

					passInfo.bufferInvalidates.add(BufferBarrier());
					BufferBarrier& invalidateBarrier = passInfo.bufferInvalidates.back();
					invalidateBarrier.stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
					invalidateBarrier.accessFlags = VK_ACCESS_INDEX_READ_BIT;
					invalidateBarrier.bufferInfoIdx = bufferInfoID;

					passInfo.bufferFlushes.add(BufferBarrier());
					BufferBarrier& flushBarrier = passInfo.bufferFlushes.back();
					flushBarrier.stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
					flushBarrier.accessFlags = 0;
					flushBarrier.bufferInfoIdx = bufferInfoID;

					updateBufferInfo(&bufferInfos[bufferInfoID], QUEUE_GRAPHIC_BIT, BUFFER_USAGE_INDEX_BIT, PassNodeID(i));
				}

				_initInShaderBuffers(graphicNode->shaderBufferReadAccesses, i, QUEUE_GRAPHIC_BIT);
				_initOutShaderBuffers(graphicNode->shaderBufferWriteAccesses, i, QUEUE_GRAPHIC_BIT);
				_initShaderTextures(graphicNode->shaderTextureReadAccesses, i, QUEUE_GRAPHIC_BIT);
				_initShaderTextures(graphicNode->shaderTextureWriteAccesses, i, QUEUE_GRAPHIC_BIT);

				for (const ColorAttachment& colorAttachment : graphicNode->colorAttachments) {
					SOUL_ASSERT(0, colorAttachment.nodeID != TEXTURE_NODE_ID_NULL, "");

					uint32 textureInfoID = getTextureInfoIndex(colorAttachment.nodeID);
					updateTextureInfo(&textureInfos[textureInfoID], QUEUE_GRAPHIC_BIT,
					                  TEXTURE_USAGE_COLOR_ATTACHMENT_BIT, passNodeID);

					TextureBarrier invalidateBarrier;
					invalidateBarrier.stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
					invalidateBarrier.accessFlags = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					invalidateBarrier.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					invalidateBarrier.textureInfoIdx = getTextureInfoIndex(colorAttachment.nodeID);
					passInfo.textureInvalidates.add(invalidateBarrier);

					TextureBarrier flushBarrier;
					flushBarrier.stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
					flushBarrier.accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					flushBarrier.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					flushBarrier.textureInfoIdx = getTextureInfoIndex(colorAttachment.nodeID);
					passInfo.textureFlushes.add(flushBarrier);
				}

				for (const InputAttachment& inputAttachment : graphicNode->inputAttachments) {

					uint32 textureInfoID = getTextureInfoIndex(inputAttachment.nodeID);
					updateTextureInfo(&textureInfos[textureInfoID], QUEUE_GRAPHIC_BIT,
					                  TEXTURE_USAGE_INPUT_ATTACHMENT_BIT, passNodeID);

					TextureBarrier invalidateBarrier;
					invalidateBarrier.stageFlags = vkCastShaderStageToPipelineStageFlags(inputAttachment.stageFlags);
					invalidateBarrier.accessFlags = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
					invalidateBarrier.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					invalidateBarrier.textureInfoIdx = textureInfoID;
					passInfo.textureInvalidates.add(invalidateBarrier);

					TextureBarrier flushBarrier;
					flushBarrier.stageFlags = vkCastShaderStageToPipelineStageFlags(inputAttachment.stageFlags);
					flushBarrier.accessFlags = 0;
					flushBarrier.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					flushBarrier.textureInfoIdx = textureInfoID;
					passInfo.textureFlushes.add(flushBarrier);
				}

				if (graphicNode->depthStencilAttachment.nodeID != TEXTURE_NODE_ID_NULL) {
					uint32 resourceInfoIndex = getTextureInfoIndex(graphicNode->depthStencilAttachment.nodeID);

					uint32 textureInfoID = getTextureInfoIndex(graphicNode->depthStencilAttachment.nodeID);
					updateTextureInfo(&textureInfos[textureInfoID], QUEUE_GRAPHIC_BIT,
					                  TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, passNodeID);

					TextureBarrier invalidateBarrier;
					invalidateBarrier.stageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
					invalidateBarrier.accessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					invalidateBarrier.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					invalidateBarrier.textureInfoIdx = resourceInfoIndex;
					passInfo.textureInvalidates.add(invalidateBarrier);

					TextureBarrier flushBarrier;
					flushBarrier.stageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
					flushBarrier.accessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					flushBarrier.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					flushBarrier.textureInfoIdx = resourceInfoIndex;
					passInfo.textureFlushes.add(flushBarrier);
				}

				break;
			}
			default:
				SOUL_NOT_IMPLEMENTED();
				break;
			}
		}

		for (const TextureExport& textureExport : _renderGraph->_textureExports) {
			TextureExecInfo& textureInfo = textureInfos[getTextureInfoIndex(textureExport.tex)];
			textureInfo.usageFlags |= TEXTURE_USAGE_TRANSFER_SRC_BIT;
			textureInfo.queueFlags |= QUEUE_GRAPHIC_BIT;
		}

		for (VkEvent& event : _externalEvents) {
			event = VK_NULL_HANDLE;
		}

		for (auto srcPassType : EnumIter<PassType>::Iterates()) {
			for (auto dstPassType : EnumIter<PassType>::Iterates()) {
				_externalSemaphores[srcPassType][dstPassType] = SEMAPHORE_ID_NULL;
			}
		}

		for (int i = 0; i < _renderGraph->_internalBuffers.size(); i++) {
			const RGInternalBuffer& rgBuffer = _renderGraph->_internalBuffers[i];
			BufferExecInfo& bufferInfo = bufferInfos[i];

			BufferDesc desc;
			desc.typeSize = rgBuffer.typeSize;
			desc.typeAlignment = rgBuffer.typeAlignment;
			desc.count = rgBuffer.count;
			desc.queueFlags = bufferInfo.queueFlags;
			desc.usageFlags = bufferInfo.usageFlags;

			bufferInfo.bufferID = _gpuSystem->bufferCreate(desc);
		}

		for (int i = 0; i < externalBufferInfos.size(); i++) {
			BufferExecInfo& bufferInfo = externalBufferInfos[i];
			if (bufferInfo.passes.size() == 0) continue;

			bufferInfo.bufferID = _renderGraph->_externalBuffers[i].bufferID;

			PassType firstPassType = _renderGraph->_passNodes[bufferInfo.passes[0].id]->type;
			ResourceOwner owner = _gpuSystem->_bufferPtr(bufferInfo.bufferID)->owner;
			PassType externalPassType = RESOURCE_OWNER_TO_PASS_TYPE_MAP[owner];
			SOUL_ASSERT(0, owner != ResourceOwner::PRESENTATION_ENGINE, "");
			if (externalPassType == firstPassType) {
				VkEvent& externalEvent = _externalEvents[firstPassType];
				if (externalEvent == VK_NULL_HANDLE) {
					externalEvent = _gpuSystem->_eventCreate();
				}
				bufferInfo.pendingEvent = externalEvent;
				bufferInfo.pendingSemaphore = SEMAPHORE_ID_NULL;
				bufferInfo.unsyncWriteStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
				bufferInfo.unsyncWriteAccess = VK_ACCESS_MEMORY_WRITE_BIT;
			} else {
				SemaphoreID& externalSemaphore = _externalSemaphores[externalPassType][firstPassType];
				if (externalSemaphore == SEMAPHORE_ID_NULL) {
					externalSemaphore = _gpuSystem->_semaphoreCreate();
				}

				bufferInfo.pendingEvent = VK_NULL_HANDLE;
				bufferInfo.pendingSemaphore = externalSemaphore;
				bufferInfo.unsyncWriteStage = 0;
				bufferInfo.unsyncWriteAccess = 0;
			}

		}

		for (int i = 0; i < _renderGraph->_internalTextures.size(); i++) {
			const RGInternalTexture& rgTexture = _renderGraph->_internalTextures[i];
			TextureExecInfo& textureInfo = textureInfos[i];

			if (textureInfo.queueFlags == 0) continue;
			
			TextureDesc desc;
			desc.width = rgTexture.width;
			desc.height = rgTexture.height;
			desc.depth = rgTexture.depth;
			desc.format = rgTexture.format;
			desc.queueFlags = textureInfo.queueFlags;
			desc.usageFlags = textureInfo.usageFlags;
			desc.mipLevels = rgTexture.mipLevels;
			desc.type = rgTexture.type;
			desc.name = rgTexture.name;
			if (!rgTexture.clear) {
				textureInfo.textureID = _gpuSystem->textureCreate(desc);
			} else {
				desc.usageFlags |= TEXTURE_USAGE_SAMPLED_BIT;
				textureInfo.textureID = _gpuSystem->textureCreate(desc, rgTexture.clearValue);
				PassType firstPassType = _renderGraph->_passNodes[textureInfo.passes[0].id]->type;
				if (firstPassType == PassType::GRAPHIC)
				{
					VkEvent& externalEvent = _externalEvents[PassType::GRAPHIC];
					if (externalEvent == VK_NULL_HANDLE) {
						externalEvent = _gpuSystem->_eventCreate();
					}
					textureInfo.pendingEvent = externalEvent;
					textureInfo.pendingSemaphore = SEMAPHORE_ID_NULL;
					textureInfo.unsyncWriteStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
					textureInfo.unsyncWriteAccess = VK_ACCESS_MEMORY_WRITE_BIT;
				} else
				{
					SemaphoreID& externalSemaphoreID = _externalSemaphores[PassType::GRAPHIC][firstPassType];
					if (externalSemaphoreID == SEMAPHORE_ID_NULL) {
						externalSemaphoreID = _gpuSystem->_semaphoreCreate();
					}
					textureInfo.pendingEvent = VK_NULL_HANDLE;
					textureInfo.pendingSemaphore = externalSemaphoreID;
					textureInfo.unsyncWriteStage = 0;
					textureInfo.unsyncWriteAccess = 0;
				}
				Texture* texture = _gpuSystem->_texturePtr(textureInfo.textureID);
			}
		}

		for (int i = 0; i < externalTextureInfos.size(); i++) {
			TextureExecInfo& textureInfo = externalTextureInfos[i];
			if (textureInfo.passes.size() == 0) continue;
			textureInfo.textureID = _renderGraph->_externalTextures[i].textureID;

			PassType firstPassType = _renderGraph->_passNodes[textureInfo.passes[0].id]->type;
			ResourceOwner owner = _gpuSystem->_texturePtr(textureInfo.textureID)->owner;
			PassType externalPassType = RESOURCE_OWNER_TO_PASS_TYPE_MAP[owner];
			if (firstPassType == PassType::NONE) {
				textureInfo.pendingEvent = VK_NULL_HANDLE;
				textureInfo.pendingSemaphore = SEMAPHORE_ID_NULL;
			} else if (owner == ResourceOwner::PRESENTATION_ENGINE) {
				textureInfo.pendingEvent = VK_NULL_HANDLE;
				textureInfo.pendingSemaphore = _gpuSystem->_frameContext().imageAvailableSemaphore;
				textureInfo.unsyncWriteStage = 0;
				textureInfo.unsyncWriteAccess = 0;
			} else if (externalPassType == firstPassType) {
				VkEvent& externalEvent = _externalEvents[firstPassType];
				if (externalEvent == VK_NULL_HANDLE) {
					externalEvent = _gpuSystem->_eventCreate();
				}
				textureInfo.pendingEvent = externalEvent;
				textureInfo.pendingSemaphore = SEMAPHORE_ID_NULL;
				textureInfo.unsyncWriteStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
				textureInfo.unsyncWriteAccess = VK_ACCESS_MEMORY_WRITE_BIT;
			} else if (owner != ResourceOwner::NONE){
				SemaphoreID& externalSemaphoreID = _externalSemaphores[externalPassType][firstPassType];
				if (externalSemaphoreID == SEMAPHORE_ID_NULL) {
					externalSemaphoreID = _gpuSystem->_semaphoreCreate();
				}
				textureInfo.pendingEvent = VK_NULL_HANDLE;
				textureInfo.pendingSemaphore = externalSemaphoreID;
				textureInfo.unsyncWriteStage = 0;
				textureInfo.unsyncWriteAccess = 0;
			}
		}
	}

	VkRenderPass RenderGraphExecution::_renderPassCreate(uint32 passIndex) {
		SOUL_PROFILE_ZONE();
		SOUL_ASSERT_MAIN_THREAD();

		auto graphicNode = (const GraphicBaseNode*) _renderGraph->_passNodes[passIndex];
		RenderPassKey renderPassKey = {};
		for (int i = 0; i < graphicNode->colorAttachments.size(); i++) {
			const ColorAttachment& attachment = graphicNode->colorAttachments[i];
			uint32 textureInfoIdx = getTextureInfoIndex(attachment.nodeID);
			const TextureExecInfo& textureInfo = textureInfos[textureInfoIdx];
			const Texture& texture = *_gpuSystem->_texturePtr(textureInfo.textureID);
			
			renderPassKey.colorAttachments[i].format = texture.format;
			if (textureInfo.firstPass.id == passIndex) renderPassKey.colorAttachments[i].flags |= ATTACHMENT_FIRST_PASS_BIT;
			if (textureInfo.lastPass.id == passIndex) renderPassKey.colorAttachments[i].flags |= ATTACHMENT_LAST_PASS_BIT;
			if (attachment.desc.clear) renderPassKey.colorAttachments[i].flags |= ATTACHMENT_CLEAR_BIT;
			if (isExternal(textureInfo)) renderPassKey.colorAttachments[i].flags |= ATTACHMENT_EXTERNAL_BIT;
			renderPassKey.colorAttachments[i].flags |= ATTACHMENT_ACTIVE_BIT;
		}

		for (int i = 0; i < graphicNode->inputAttachments.size(); i++) {
			const InputAttachment& attachment = graphicNode->inputAttachments[i];
			uint32 textureInfoIdx = getTextureInfoIndex(attachment.nodeID);
			const TextureExecInfo& textureInfo = textureInfos[textureInfoIdx];
			const Texture& texture = *_gpuSystem->_texturePtr(textureInfo.textureID);

			Attachment& attachmentKey = renderPassKey.inputAttachments[i];
			attachmentKey.format = texture.format;
			if (textureInfo.firstPass.id == passIndex) attachmentKey.flags |= ATTACHMENT_FIRST_PASS_BIT;
			if (textureInfo.lastPass.id == passIndex) attachmentKey.flags |= ATTACHMENT_LAST_PASS_BIT;
			if (isExternal(textureInfo)) attachmentKey.flags |= ATTACHMENT_EXTERNAL_BIT;
			attachmentKey.flags |= ATTACHMENT_ACTIVE_BIT;
		}

		if (graphicNode->depthStencilAttachment.nodeID != TEXTURE_NODE_ID_NULL) {
			const DepthStencilAttachment& attachment = graphicNode->depthStencilAttachment;

			uint32 textureInfoIdx = getTextureInfoIndex(graphicNode->depthStencilAttachment.nodeID);
			const TextureExecInfo& textureInfo = textureInfos[textureInfoIdx];
			const Texture& texture = *_gpuSystem->_texturePtr(textureInfo.textureID);

			renderPassKey.depthAttachment.format = texture.format;
			if (textureInfo.firstPass.id == passIndex) renderPassKey.depthAttachment.flags |= ATTACHMENT_FIRST_PASS_BIT;
			if (textureInfo.lastPass.id == passIndex) renderPassKey.depthAttachment.flags |= ATTACHMENT_LAST_PASS_BIT;
			if (attachment.desc.clear) renderPassKey.depthAttachment.flags |= ATTACHMENT_CLEAR_BIT;
			if (isExternal(textureInfo)) renderPassKey.depthAttachment.flags |= ATTACHMENT_EXTERNAL_BIT;
			renderPassKey.depthAttachment.flags |= ATTACHMENT_ACTIVE_BIT;
		}
		
		return _gpuSystem->_renderPassRequest(renderPassKey);
	}

	VkFramebuffer RenderGraphExecution::_framebufferCreate(uint32 passIndex, VkRenderPass renderPass) {

		SOUL_ASSERT_MAIN_THREAD();

		auto graphicNode = (const GraphicBaseNode*) _renderGraph->_passNodes[passIndex];

		VkImageView imageViews[MAX_COLOR_ATTACHMENT_PER_SHADER + 1];
		int imageViewCount = 0;
		for (const ColorAttachment& attachment : graphicNode->colorAttachments) {
			const Texture* texture = getTexture(attachment.nodeID);
			imageViews[imageViewCount++] = texture->view;
		}

		for (const InputAttachment& attachment : graphicNode->inputAttachments) {
			const Texture* texture = getTexture(attachment.nodeID);
			imageViews[imageViewCount++] = texture->view;
		}

		if (graphicNode->depthStencilAttachment.nodeID != TEXTURE_NODE_ID_NULL) {
			const Texture* texture = getTexture(graphicNode->depthStencilAttachment.nodeID);
			imageViews[imageViewCount++] = texture->view;
		}

		VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = imageViewCount;
		framebufferInfo.pAttachments = imageViews;
		framebufferInfo.width = graphicNode->renderTargetDimension.x;
		framebufferInfo.height = graphicNode->renderTargetDimension.y;
		framebufferInfo.layers = 1;

		return _gpuSystem->_framebufferCreate(framebufferInfo);
	}

	void RenderGraphExecution::_submitExternalSyncPrimitive() {

		// Sync semaphores
		for(const auto srcPassType : EnumIter<PassType>::Iterates()) {
			Runtime::ScopeAllocator scopeAllocator("Sync semaphore allocator", Runtime::GetTempAllocator());
			Array<Semaphore*> semaphores(&scopeAllocator);
			semaphores.reserve(uint64(PassType::COUNT));

			const QueueType srcQueueType = PASS_TYPE_TO_QUEUE_TYPE_MAP[srcPassType];

			for (const auto dstPassType : EnumIter<PassType>::Iterates()) {
				if (_externalSemaphores[srcPassType][dstPassType] != SEMAPHORE_ID_NULL) {
					semaphores.add(_gpuSystem->_semaphorePtr(_externalSemaphores[srcPassType][dstPassType]));
				}
			}
			if (!semaphores.empty()) {
				VkCommandBuffer syncSemaphoreCmdBuffer = commandPools.requestCommandBuffer(srcQueueType);
				commandQueues[srcQueueType].submit(syncSemaphoreCmdBuffer, semaphores);
			}
		}

		// Sync events
		for(PassType passType : EnumIter<PassType>::Iterates()) {
			QueueType queueType = PASS_TYPE_TO_QUEUE_TYPE_MAP[passType];
			if (_externalEvents[passType] != VK_NULL_HANDLE && passType != PassType::NONE) {
				VkCommandBuffer syncEventCmdBuffer = commandPools.requestCommandBuffer(queueType);
				vkCmdSetEvent(syncEventCmdBuffer, _externalEvents[passType], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
				commandQueues[queueType].submit(syncEventCmdBuffer);
			}
		}

	}

	void RenderGraphExecution::_executePass(uint32 passIndex, VkCommandBuffer cmdBuffer) {
		SOUL_PROFILE_ZONE();
		PassNode* passNode = _renderGraph->_passNodes[passIndex];
		// Run render pass here
		switch(passNode->type) {
		case PassType::NONE:
			SOUL_PANIC(0, "Pass Type is unknown!");
			break;
		case PassType::TRANSFER:
			SOUL_NOT_IMPLEMENTED();
			break;
		case PassType::COMPUTE: {
			/*auto computeNode = (const ComputeBaseNode*) passNode;
                ProgramDesc programKey;
                programKey.shaderIDs[ShaderStage::COMPUTE] = computeNode->pipelineConfig.computeShaderID;
                ProgramID programID = _gpuSystem->programRequest(programKey);
                VkPipeline pipeline = _gpuSystem->_pipelineCreate(*computeNode, programID);
                vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
                CommandBucket commandBucket;
                RenderGraphRegistry registry(_gpuSystem, this, VK_NULL_HANDLE);
                passNode->executePass(&registry, &commandBucket);

				{
					SOUL_PROFILE_ZONE_WITH_NAME("Compute command submission");
					for (Command::Command* command : commandBucket.commands) {
						command->_submit(&_gpuSystem->_db, programID, cmdBuffer);
					}
				}
                
                _gpuSystem->_pipelineDestroy(pipeline);*/

			break;
		}
		case PassType::GRAPHIC: {
			auto graphicNode = (const GraphicBaseNode*) passNode;
			VkRenderPass renderPass = _renderPassCreate(passIndex);

			VkFramebuffer framebuffer = _framebufferCreate(passIndex, renderPass);

			VkRenderPassBeginInfo renderPassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
			renderPassBeginInfo.renderPass = renderPass;
			renderPassBeginInfo.framebuffer = framebuffer;
			renderPassBeginInfo.renderArea.offset = {0, 0};
			renderPassBeginInfo.renderArea.extent = {
				graphicNode->renderTargetDimension.x,
				graphicNode->renderTargetDimension.y
			};


			VkClearValue clearValues[MAX_COLOR_ATTACHMENT_PER_SHADER + 1] = {};
			uint32 clearCount = 0;

			for (ColorAttachment attachment : graphicNode->colorAttachments) {
				ClearValue clearValue = attachment.desc.clearValue;
				memcpy(&clearValues[clearCount++], &clearValue, sizeof(VkClearValue));
			}

			for (InputAttachment attachment : graphicNode->inputAttachments) {
				clearValues[clearCount++] = {};
			}

			if (graphicNode->depthStencilAttachment.nodeID != TEXTURE_NODE_ID_NULL) {
				const DepthStencilAttachmentDesc& desc = graphicNode->depthStencilAttachment.desc;
				ClearValue clearValue = desc.clearValue;
				clearValues[clearCount++].depthStencil = { clearValue.depthStencil.depth, clearValue.depthStencil.stencil };
			}

			renderPassBeginInfo.clearValueCount = clearCount;
			renderPassBeginInfo.pClearValues = clearValues;

			RenderCommandBucket commandBucket;
			RenderGraphRegistry registry(_gpuSystem, this, renderPass);
			passNode->executePass(&registry, &commandBucket);

			// TODO(kevinyu) : Make this threshold configurable
			static uint32 SECONDARY_COMMAND_BUFFER_THRESHOLD = 10;
			if (commandBucket.commands.size() > SECONDARY_COMMAND_BUFFER_THRESHOLD)
			{
				SOUL_PROFILE_ZONE_WITH_NAME("ST Graphic Command Submission");
				vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				RenderCompiler renderCompiler(_gpuSystem, cmdBuffer);
				for (const RenderCommand* command : commandBucket.commands)
				{
					renderCompiler.compileCommand(*command);
				}
				vkCmdEndRenderPass(cmdBuffer);
			}
			else
			{
				vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
				Array<VkCommandBuffer> secondaryCommandBuffers;
				secondaryCommandBuffers.resize(std::min<soul_size>(Runtime::GetThreadCount(), ((commandBucket.commands.size() + 99)/ 100)));

				{
					SOUL_PROFILE_ZONE_WITH_NAME("MT Commands translation Master");

					struct CommandTranslationData
					{
						Array<VkCommandBuffer>* cmdBuffers;
						RenderCommandBucket* cmdBucket;
						VkRenderPass renderPass;
						VkFramebuffer framebuffer;
						CommandPools& commandPools;
						GPU::System* gpuSystem;
					} taskData = {
						&secondaryCommandBuffers,
						&commandBucket,
						renderPass,
						framebuffer,
						commandPools,
						_gpuSystem
					};

					Runtime::TaskID commandTranslationJob = Runtime::ParallelForTaskCreate(
						Runtime::TaskID::ROOT(), secondaryCommandBuffers.size(), 1,
						[&taskData](int index)
						{
							SOUL_PROFILE_ZONE_WITH_NAME("Command Translation Child");
							VkCommandBuffer secCmdBuffer = taskData.commandPools.requestSecondaryCommandBuffer();
							VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
							beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

							VkCommandBufferInheritanceInfo inheritanceInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
							inheritanceInfo.renderPass = taskData.renderPass;
							inheritanceInfo.subpass = 0;
							inheritanceInfo.framebuffer = taskData.framebuffer;

							beginInfo.pInheritanceInfo = &inheritanceInfo;

							vkBeginCommandBuffer(secCmdBuffer, &beginInfo);

							uint32 div = taskData.cmdBucket->commands.size() / taskData.cmdBuffers->size();
							uint32 mod = taskData.cmdBucket->commands.size() % taskData.cmdBuffers->size();

							RenderCompiler renderCompiler(taskData.gpuSystem, secCmdBuffer);

							if (index < mod)
							{
								int start = index * (div + 1);

								for (int i = 0; i < div + 1; i++)
								{
									renderCompiler.compileCommand(*taskData.cmdBucket->commands[start + i]);
								}
							}  
							else
							{
								int start = mod * (div + 1) + (index - mod) * div;
								for (int i = 0; i < div; i++)
								{
									renderCompiler.compileCommand(*taskData.cmdBucket->commands[start + i]);
								}
							}


							vkEndCommandBuffer(secCmdBuffer);
							(*taskData.cmdBuffers)[index] = secCmdBuffer;
						});

					Runtime::RunTask(commandTranslationJob);
					Runtime::WaitTask(commandTranslationJob);

				}

				vkCmdExecuteCommands(cmdBuffer, secondaryCommandBuffers.size(), secondaryCommandBuffers.data());
				vkCmdEndRenderPass(cmdBuffer);
			}

			_gpuSystem->_framebufferDestroy(framebuffer);

			break;
		}
		default: {
			SOUL_NOT_IMPLEMENTED();
			break;
		}
		}
	}

	void RenderGraphExecution::run() {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE();

		_submitExternalSyncPrimitive();

		Array<VkEvent> garbageEvents;
		Array<SemaphoreID> garbageSemaphores;

		Array<VkBufferMemoryBarrier> eventBufferBarriers;
		Array<VkImageMemoryBarrier> eventImageBarriers;

		Array<VkImageMemoryBarrier> initLayoutBarriers;
		Array<VkImageMemoryBarrier> semaphoreLayoutBarriers;
		Array<VkEvent> events;

		static auto needInvalidate = [](VkAccessFlags visibleAccessMatrix[],
		                                VkAccessFlags accessFlags, VkPipelineStageFlags stageFlags) -> bool {
			bool result = false;
			Util::ForEachBit(stageFlags, [&result, accessFlags, visibleAccessMatrix](uint32 bit) {
				if (accessFlags & ~visibleAccessMatrix[bit]) {
					result = true;
				}
			});
			return result;
		};

		for (int i = 0; i < _renderGraph->_passNodes.size(); i++) {
			Runtime::ScopeAllocator passNodeScopeAllocator("Pass Node Scope Allocator", Runtime::GetTempAllocator());
			PassNode* passNode = _renderGraph->_passNodes[i];
			QueueType queueType = PASS_TYPE_TO_QUEUE_TYPE_MAP[passNode->type];
			PassExecInfo& passInfo = passInfos[i];

			VkCommandBuffer cmdBuffer = commandPools.requestCommandBuffer(queueType);

			Vec4f color = { (rand() % 125) / 255.0f, (rand() % 125) / 255.0f, (rand() % 125) / 255.0f, 1.0f };
			const VkDebugUtilsLabelEXT passLabel = {
				VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, // sType
				nullptr,                                  // pNext
				passNode->name,                           // pLabelName
				{color.x, color.y, color.z, color.w},             // color
			};
			vkCmdBeginDebugUtilsLabelEXT(cmdBuffer, &passLabel);
			
			eventBufferBarriers.resize(0);
			eventImageBarriers.resize(0);
			initLayoutBarriers.resize(0);
			semaphoreLayoutBarriers.resize(0);
			events.resize(0);

			VkPipelineStageFlags eventSrcStageFlags = 0;
			VkPipelineStageFlags eventDstStageFlags = 0;
			VkPipelineStageFlags semaphoreDstStageFlags = 0;
			VkPipelineStageFlags initLayoutDstStageFlags = 0;

			for (const BufferBarrier& barrier: passInfo.bufferInvalidates) {
				BufferExecInfo& bufferInfo = bufferInfos[barrier.bufferInfoIdx];

				if (bufferInfo.unsyncWriteAccess != 0) {
					for (VkAccessFlags& accessFlags : bufferInfo.visibleAccessMatrix) {
						accessFlags = 0;
					}
				}

				if (bufferInfo.pendingSemaphore == SEMAPHORE_ID_NULL &&
					bufferInfo.unsyncWriteAccess == 0 &&
					!needInvalidate(bufferInfo.visibleAccessMatrix, barrier.accessFlags, barrier.stageFlags)) {
					continue;
				}

				VkBufferMemoryBarrier memBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
				memBarrier.srcAccessMask = bufferInfo.unsyncWriteAccess;
				memBarrier.dstAccessMask = barrier.accessFlags;
				memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				memBarrier.buffer = _gpuSystem->_bufferPtr(bufferInfo.bufferID)->vkHandle;
				memBarrier.offset = 0;
				memBarrier.size = VK_WHOLE_SIZE;

				if (bufferInfo.pendingSemaphore != SEMAPHORE_ID_NULL) {
					SOUL_ASSERT(0, bufferInfo.pendingSemaphore != SEMAPHORE_ID_NULL, "");
					Semaphore& semaphore = *_gpuSystem->_semaphorePtr(bufferInfo.pendingSemaphore);

					if (!semaphore.isPending()) {
						commandQueues[queueType].wait(_gpuSystem->_semaphorePtr(bufferInfo.pendingSemaphore), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
					}

					bufferInfo.pendingSemaphore = SEMAPHORE_ID_NULL;
				} else {
					VkAccessFlags dstAccessFlags = barrier.accessFlags;

					eventBufferBarriers.add(memBarrier);
					events.add(bufferInfo.pendingEvent);
					eventSrcStageFlags |= bufferInfo.unsyncWriteStage;
					eventDstStageFlags |= barrier.stageFlags;

					Util::ForEachBit(barrier.stageFlags, [&bufferInfo, dstAccessFlags](uint32 bit) {
						bufferInfo.visibleAccessMatrix[bit] |= dstAccessFlags;
					});

					bufferInfo.pendingEvent = VK_NULL_HANDLE;

				}
			}

			for (const TextureBarrier& barrier: passInfo.textureInvalidates) {
				TextureExecInfo& textureInfo = textureInfos[barrier.textureInfoIdx];
				Texture* texture = _gpuSystem->_texturePtr(textureInfo.textureID);

				bool layoutChange = texture->layout != barrier.layout;

				if (textureInfo.unsyncWriteAccess != 0 || layoutChange) {
					for (VkAccessFlags& accessFlags : textureInfo.visibleAccessMatrix) {
						accessFlags = 0;
					}
				}

				if (textureInfo.pendingSemaphore == SEMAPHORE_ID_NULL &&
					!layoutChange &&
					textureInfo.unsyncWriteAccess == 0 &&
					!needInvalidate(textureInfo.visibleAccessMatrix, barrier.accessFlags, barrier.stageFlags)) {
					continue;
				}

				VkImageMemoryBarrier memBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
				memBarrier.oldLayout = texture->layout;
				memBarrier.newLayout = barrier.layout;
				memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				memBarrier.image = texture->vkHandle;
				memBarrier.subresourceRange.aspectMask = vkCastFormatToAspectFlags(texture->format);
				memBarrier.subresourceRange.baseMipLevel = 0;
				memBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
				memBarrier.subresourceRange.baseArrayLayer = 0;
				memBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

				if (textureInfo.pendingSemaphore != SEMAPHORE_ID_NULL) {
					semaphoreDstStageFlags |= barrier.stageFlags;
					if (layoutChange) {

						memBarrier.srcAccessMask = 0;
						memBarrier.dstAccessMask = barrier.accessFlags;
						semaphoreLayoutBarriers.add(memBarrier);

					}
					Semaphore& semaphore = *_gpuSystem->_semaphorePtr(textureInfo.pendingSemaphore);
					if (!semaphore.isPending()) {
						commandQueues[queueType].wait(_gpuSystem->_semaphorePtr(textureInfo.pendingSemaphore), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
					}
					textureInfo.pendingSemaphore = SEMAPHORE_ID_NULL;
				} else if (textureInfo.pendingEvent != VK_NULL_HANDLE){
					VkAccessFlags dstAccessFlags = barrier.accessFlags;

					memBarrier.srcAccessMask = textureInfo.unsyncWriteAccess;
					memBarrier.dstAccessMask = dstAccessFlags;

					eventImageBarriers.add(memBarrier);
					events.add(textureInfo.pendingEvent);
					eventSrcStageFlags |= textureInfo.unsyncWriteStage;
					eventDstStageFlags |= barrier.stageFlags;

					Util::ForEachBit(barrier.stageFlags, [&textureInfo, dstAccessFlags](uint32 bit) {
						textureInfo.visibleAccessMatrix[bit] |= dstAccessFlags;
					});

					textureInfo.pendingEvent = VK_NULL_HANDLE;

				} else {
					SOUL_ASSERT(0, layoutChange, "");
					SOUL_ASSERT(0, texture->layout == VK_IMAGE_LAYOUT_UNDEFINED, "");

					memBarrier.srcAccessMask = 0;
					memBarrier.dstAccessMask = barrier.accessFlags;
					initLayoutBarriers.add(memBarrier);
					initLayoutDstStageFlags |= barrier.stageFlags;
				}

				texture->layout = barrier.layout;
			}

			if (events.size() > 0) {
				vkCmdWaitEvents(cmdBuffer,
				                events.size(), events.data(),
				                eventSrcStageFlags, eventDstStageFlags,
				                0, nullptr,
				                eventBufferBarriers.size(), eventBufferBarriers.data(),
				                eventImageBarriers.size(), eventImageBarriers.data()
				);
			}

			if (initLayoutBarriers.size() > 0) {
				vkCmdPipelineBarrier(cmdBuffer,
				                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, initLayoutDstStageFlags,
				                     0,
				                     0, nullptr,
				                     0, nullptr,
				                     initLayoutBarriers.size(), initLayoutBarriers.data());
			}

			if (semaphoreLayoutBarriers.size() > 0) {
				vkCmdPipelineBarrier(cmdBuffer,
				                     semaphoreDstStageFlags, semaphoreDstStageFlags,
				                     0,
				                     0, nullptr,
				                     0, nullptr,
				                     semaphoreLayoutBarriers.size(), semaphoreLayoutBarriers.data());
			}

			_executePass(i, cmdBuffer);

			EnumArray<PassType, bool> isPassTypeDependent(false);
			for (const BufferBarrier& barrier : passInfo.bufferFlushes) {
				BufferExecInfo& bufferInfo = bufferInfos[barrier.bufferInfoIdx];
				if (bufferInfo.passCounter != bufferInfo.passes.size() - 1) {
					uint32 nextPassIdx = bufferInfo.passes[bufferInfo.passCounter + 1].id;
					PassType passType = _renderGraph->_passNodes[nextPassIdx]->type;
					isPassTypeDependent[passType] = true;
				}
			}

			for (const TextureBarrier& barrier: passInfo.textureFlushes) {
				TextureExecInfo& textureInfo = textureInfos[barrier.textureInfoIdx];
				if (textureInfo.passCounter != textureInfo.passes.size() - 1) {
					uint32 nextPassIdx = textureInfo.passes[textureInfo.passCounter + 1].id;
					PassType passType = _renderGraph->_passNodes[nextPassIdx]->type;
					isPassTypeDependent[passType] = true;
				}
			}

			EnumArray<PassType, SemaphoreID> semaphoresMap(SEMAPHORE_ID_NULL);
			VkEvent event = VK_NULL_HANDLE;
			VkPipelineStageFlags eventStageFlags = 0;

			for (PassType passType : EnumIter<PassType>::Iterates()) {
				if (isPassTypeDependent[passType] && passType == passNode->type) {
					event = _gpuSystem->_eventCreate();
					garbageEvents.add(event);
				} else if (isPassTypeDependent[passType] && passType != passNode->type) {
					semaphoresMap[passType] = _gpuSystem->_semaphoreCreate();
					garbageSemaphores.add(semaphoresMap[passType]);
				}
			}

			for (const BufferBarrier& barrier : passInfo.bufferFlushes) {
				BufferExecInfo& bufferInfo = bufferInfos[barrier.bufferInfoIdx];
				if (bufferInfo.passCounter != bufferInfo.passes.size() - 1) {
					uint32 nextPassIdx = bufferInfo.passes[bufferInfo.passCounter + 1].id;
					PassType nextPassType = _renderGraph->_passNodes[nextPassIdx]->type;

					if (passNode->type != nextPassType) {
						SemaphoreID semaphoreID = semaphoresMap[nextPassType];
						SOUL_ASSERT(0, semaphoreID != SEMAPHORE_ID_NULL, "");
						bufferInfo.pendingSemaphore = semaphoreID;
					} else {
						SOUL_ASSERT(0, event != VK_NULL_HANDLE, "");
						bufferInfo.pendingEvent = event;
						eventStageFlags |= barrier.stageFlags;
						
					}
				}
			}

			auto textureExported = [this](uint16 textureInfoIdx, const Array<TextureExport>& exports) -> bool {
				for (const TextureExport& texExport : exports) {
					if (getTextureInfoIndex(texExport.tex) == textureInfoIdx) {
						return true;
					}
				}
				return false;
			};

			for (const TextureBarrier& barrier: passInfo.textureFlushes) {
				TextureExecInfo& textureInfo = textureInfos[barrier.textureInfoIdx];
				if (textureInfo.passCounter != textureInfo.passes.size() - 1) {
					uint32 nextPassIdx = textureInfo.passes[textureInfo.passCounter + 1].id;
					PassType nextPassType = _renderGraph->_passNodes[nextPassIdx]->type;
					if (passNode->type != nextPassType) {
						SemaphoreID semaphoreID = semaphoresMap[nextPassType];
						SOUL_ASSERT(0, semaphoreID != SEMAPHORE_ID_NULL, "");
						textureInfo.pendingSemaphore = semaphoreID;
					} else {
						SOUL_ASSERT(0, event != VK_NULL_HANDLE, "");
						textureInfo.pendingEvent = event;
						eventStageFlags |= barrier.stageFlags;
					}
				}
				_gpuSystem->_texturePtr(textureInfo.textureID)->layout = barrier.layout;
			}

			if (event != VK_NULL_HANDLE) {
				vkCmdSetEvent(cmdBuffer, event, eventStageFlags);
			}

			Array<Semaphore*> semaphores(&passNodeScopeAllocator);
			semaphores.reserve(uint64(PassType::COUNT));
			for (SemaphoreID semaphoreID : semaphoresMap) {
				if (semaphoreID != SEMAPHORE_ID_NULL) {
					semaphores.add(_gpuSystem->_semaphorePtr(semaphoreID));
				}
			}

			// Update unsync stage
			for (const BufferBarrier& barrier : passInfo.bufferFlushes) {
				BufferExecInfo& bufferInfo = bufferInfos[barrier.bufferInfoIdx];
				if (bufferInfo.passCounter != bufferInfo.passes.size() - 1) {
					uint32 nextPassIdx = bufferInfo.passes[++bufferInfo.passCounter].id;
					PassType nextPassType = _renderGraph->_passNodes[nextPassIdx]->type;
					if (passNode->type != nextPassType) {
						bufferInfo.unsyncWriteAccess = 0;
						bufferInfo.unsyncWriteStage = 0;
					} else {
						SOUL_ASSERT(0, event != VK_NULL_HANDLE, "");
						bufferInfo.unsyncWriteAccess = barrier.accessFlags;
						bufferInfo.unsyncWriteStage = eventStageFlags;
					}
				}
			}

			for (const TextureBarrier& barrier: passInfo.textureFlushes) {
				TextureExecInfo& textureInfo = textureInfos[barrier.textureInfoIdx];
				if (textureInfo.passCounter != textureInfo.passes.size() - 1) {
					uint32 nextPassIdx = textureInfo.passes[++textureInfo.passCounter].id;
					PassType nextPassType = _renderGraph->_passNodes[nextPassIdx]->type;
					if (passNode->type != nextPassType) {
						textureInfo.unsyncWriteStage = 0;
						textureInfo.unsyncWriteAccess = 0;
					} else {
						SOUL_ASSERT(0, event != VK_NULL_HANDLE, "");
						textureInfo.unsyncWriteStage = eventStageFlags;
						textureInfo.unsyncWriteAccess = barrier.accessFlags;
					}
				}
			}
			vkCmdEndDebugUtilsLabelEXT(cmdBuffer);
			commandQueues[queueType].submit(cmdBuffer, semaphores);
		}

		for (const TextureExport& texExport : _renderGraph->_textureExports) {

			TextureExecInfo& textureInfo = textureInfos[getTextureInfoIndex(texExport.tex)];
			TextureID dstTexID = _gpuSystem->_textureExportCreate(textureInfo.textureID);
			Texture* dstTexture = _gpuSystem->_texturePtr(dstTexID);
			Texture* srcTexture = _gpuSystem->_texturePtr(textureInfo.textureID);

			QueueType queueType = QueueType::GRAPHIC;
			VkCommandBuffer cmdBuffer = commandPools.requestCommandBuffer(queueType);
			
			VkImageMemoryBarrier srcMemBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			srcMemBarrier.oldLayout = srcTexture->layout;
			srcMemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			srcMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			srcMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			srcMemBarrier.image = srcTexture->vkHandle;
			srcMemBarrier.subresourceRange.aspectMask = vkCastFormatToAspectFlags(srcTexture->format);
			srcMemBarrier.subresourceRange.baseMipLevel = 0;
			srcMemBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			srcMemBarrier.subresourceRange.baseArrayLayer = 0;
			srcMemBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
			
			srcMemBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
			srcMemBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &srcMemBarrier
			);

			srcMemBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			srcMemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &srcMemBarrier
			);

			/*VkPipelineStageFlags eventSrcStageFlags = 0;
			VkPipelineStageFlags eventDstStageFlags = 0;

			if (textureInfo.pendingSemaphore != SEMAPHORE_ID_NULL) {
				SOUL_NOT_IMPLEMENTED();
			}
			else if (textureInfo.pendingEvent != VK_NULL_HANDLE) {
				VkAccessFlags dstAccessFlags = accessFlags;

				srcMemBarrier.srcAccessMask = textureInfo.unsyncWriteAccess;
				srcMemBarrier.dstAccessMask = dstAccessFlags;

				eventImageBarriers.add(srcMemBarrier);
				events.add(textureInfo.pendingEvent);
				eventSrcStageFlags |= textureInfo.unsyncWriteStage;
				eventDstStageFlags |= stageFlags;

				Util::ForEachBit(stageFlags, [&textureInfo, dstAccessFlags](uint32 bit) {
					textureInfo.visibleAccessMatrix[bit] |= dstAccessFlags;
					});

				textureInfo.pendingEvent = VK_NULL_HANDLE;
			}
			else {
				VkAccessFlags dstAccessFlags = accessFlags;

				srcMemBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
				srcMemBarrier.dstAccessMask = dstAccessFlags;

				eventImageBarriers.add(srcMemBarrier);
				eventSrcStageFlags |= textureInfo.unsyncWriteStage;
				eventDstStageFlags |= stageFlags;

				Util::ForEachBit(stageFlags, [&textureInfo, dstAccessFlags](uint32 bit) {
					textureInfo.visibleAccessMatrix[bit] |= dstAccessFlags;
				});
			} */

			srcTexture->layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			
			VkImageMemoryBarrier dstMemBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			dstMemBarrier.oldLayout = dstTexture->layout;
			dstMemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			dstMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			dstMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			dstMemBarrier.image = dstTexture->vkHandle;
			dstMemBarrier.subresourceRange.aspectMask = vkCastFormatToAspectFlags(dstTexture->format);
			dstMemBarrier.subresourceRange.baseMipLevel = 0;
			dstMemBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			dstMemBarrier.subresourceRange.baseArrayLayer = 0;
			dstMemBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
			dstMemBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
			dstMemBarrier.dstAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &dstMemBarrier);

			VkImageCopy imageCopyRegion{};
			imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopyRegion.srcSubresource.layerCount = 1;
			imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopyRegion.dstSubresource.layerCount = 1;
			imageCopyRegion.extent.width = srcTexture->extent.width;
			imageCopyRegion.extent.height = srcTexture->extent.height;
			imageCopyRegion.extent.depth = 1;

			// Issue the copy command
			vkCmdCopyImage(
				cmdBuffer,
				srcTexture->vkHandle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				dstTexture->vkHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&imageCopyRegion);

			VkImageMemoryBarrier dstAfterMemBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			dstAfterMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			dstAfterMemBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			dstAfterMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			dstAfterMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			dstAfterMemBarrier.image = dstTexture->vkHandle;
			dstAfterMemBarrier.subresourceRange.aspectMask = vkCastFormatToAspectFlags(dstTexture->format);
			dstAfterMemBarrier.subresourceRange.baseMipLevel = 0;
			dstAfterMemBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			dstAfterMemBarrier.subresourceRange.baseArrayLayer = 0;
			dstAfterMemBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
			dstAfterMemBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
			dstAfterMemBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_HOST_READ_BIT;

			vkCmdPipelineBarrier(
				cmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &dstAfterMemBarrier);

			if (isExternal(textureInfo)) {
				SOUL_NOT_IMPLEMENTED();
				// Insert transition barrier to original format for srcTexture
			}


			auto t1 = std::chrono::high_resolution_clock::now();
			VkFence exportFence;
			VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
			SOUL_VK_CHECK(vkCreateFence(_gpuSystem->_db.device, &fenceInfo, nullptr, &exportFence), "");
			commandQueues[queueType].submit(cmdBuffer, 0, nullptr, exportFence);
			SOUL_VK_CHECK(vkWaitForFences(_gpuSystem->_db.device, 1, &exportFence, VK_TRUE, 100000000000));
			auto t2 = std::chrono::high_resolution_clock::now();

			auto ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
			SOUL_LOG_INFO("Delta submit command buffer: %d", ms1);
			
			vkDestroyFence(_gpuSystem->_db.device, exportFence, nullptr);

			VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
			VkSubresourceLayout subResourceLayout;
			vkGetImageSubresourceLayout(_gpuSystem->_db.device, dstTexture->vkHandle, &subResource, &subResourceLayout);


			auto t3 = std::chrono::high_resolution_clock::now();
			// Map image memory so we can start copying from it
			SOUL_LOG_INFO("Frame counter = %d, Texture ID = %d", _gpuSystem->_db.frameCounter, dstTexID);
			char* data;
			vmaMapMemory(_gpuSystem->_db.gpuAllocator, dstTexture->allocation, (void**)&data);
			data += subResourceLayout.offset;
			
			char* dstPixels = texExport.pixels;
			uint16 extentWidth = dstTexture->extent.width;
			uint16 rowPitch = subResourceLayout.rowPitch;

			auto copyPixelsJob = Runtime::ParallelForTaskCreate(
				Runtime::TaskID::ROOT(), dstTexture->extent.height, 16,
				[data, dstPixels, extentWidth, rowPitch](int index) {
					uint32 pixelOffset = 4 * extentWidth * index;
					memcpy(dstPixels + pixelOffset, data + rowPitch * index, 4 * extentWidth);
				}
			);

			Runtime::RunTask(copyPixelsJob);
			Runtime::WaitTask(copyPixelsJob);

			vmaUnmapMemory(_gpuSystem->_db.gpuAllocator, dstTexture->allocation);
			auto t4 = std::chrono::high_resolution_clock::now();
			auto ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
			SOUL_LOG_INFO("Delta submit command buffer: %d", ms2);


			_gpuSystem->textureDestroy(dstTexID);
		}

		// Update resource owner for external resource
		static constexpr EnumArray<PassType, ResourceOwner> PASS_TYPE_TO_RESOURCE_OWNER({
			ResourceOwner::NONE,
			ResourceOwner::GRAPHIC_QUEUE,
			ResourceOwner::COMPUTE_QUEUE,
			ResourceOwner::TRANSFER_QUEUE
		});

		for (const TextureExecInfo& textureInfo : textureInfos) {
			if (textureInfo.passes.size() == 0) continue;
			Texture* texture = _gpuSystem->_texturePtr(textureInfo.textureID);
			uint64 lastPassIdx = textureInfo.passes.back().id;
			PassType lastPassType = _renderGraph->_passNodes[lastPassIdx]->type;
			texture->owner = PASS_TYPE_TO_RESOURCE_OWNER[lastPassType];
		}

		for (const BufferExecInfo& bufferInfo : bufferInfos) {
			if (bufferInfo.passes.size() == 0) continue;
			Buffer* buffer = _gpuSystem->_bufferPtr(bufferInfo.bufferID);
			uint64 lastPassIdx = bufferInfo.passes.back().id;
			PassType lastPassType = _renderGraph->_passNodes[lastPassIdx]->type;
			buffer->owner = PASS_TYPE_TO_RESOURCE_OWNER[lastPassType];
		}

		for (VkEvent event : garbageEvents) {
			_gpuSystem->_eventDestroy(event);
		}

		for (SemaphoreID semaphoreID : garbageSemaphores) {
			_gpuSystem->_semaphoreDestroy(semaphoreID);
		}

		garbageEvents.cleanup();
		garbageSemaphores.cleanup();

		eventBufferBarriers.cleanup();
		eventImageBarriers.cleanup();
		initLayoutBarriers.cleanup();
		semaphoreLayoutBarriers.cleanup();
		events.cleanup();

	}

	bool RenderGraphExecution::isExternal(const BufferExecInfo& info) const {
		return (&info - bufferInfos.data()) >= _renderGraph->_internalBuffers.size();
	}

	bool RenderGraphExecution::isExternal(const TextureExecInfo& info) const {
		return (&info - textureInfos.data()) >= _renderGraph->_internalTextures.size();
	}

	BufferID RenderGraphExecution::getBufferID(BufferNodeID nodeID) const {
		uint32 infoIdx = getBufferInfoIndex(nodeID);
		const BufferExecInfo& bufferInfo = bufferInfos[infoIdx];
		return bufferInfo.bufferID;
	}

	TextureID RenderGraphExecution::getTextureID(TextureNodeID nodeID) const {
		uint32 infoIdx = getTextureInfoIndex(nodeID);
		const TextureExecInfo& textureInfo = textureInfos[infoIdx];
		return textureInfo.textureID;
	}

	Buffer* RenderGraphExecution::getBuffer(BufferNodeID nodeID) const {
		uint32 infoIdx = getBufferInfoIndex(nodeID);
		const BufferExecInfo& bufferInfo = bufferInfos[infoIdx];
		return _gpuSystem->_bufferPtr(bufferInfo.bufferID);
	}

	Texture* RenderGraphExecution::getTexture(TextureNodeID nodeID) const {
		uint32 infoIdx = getTextureInfoIndex(nodeID);
		const TextureExecInfo& textureInfo = textureInfos[infoIdx];
		return _gpuSystem->_texturePtr(textureInfo.textureID);
	}

	uint32 RenderGraphExecution::getBufferInfoIndex(BufferNodeID nodeID) const {
		const BufferNode* node = _renderGraph->_bufferNodePtr(nodeID);
		if (node->resourceID.isExternal()) {
			return _renderGraph->_internalBuffers.size() + node->resourceID.getIndex();
		}
		return node->resourceID.getIndex();
	}

	uint32 RenderGraphExecution::getTextureInfoIndex(TextureNodeID nodeID) const {
		const TextureNode* node = _renderGraph->_textureNodePtr(nodeID);
		if (node->resourceID.isExternal()) {
			return _renderGraph->_internalTextures.size() + node->resourceID.getIndex();
		}
		return node->resourceID.getIndex();
	}

	void RenderGraphExecution::cleanup() {

		for (VkEvent event : _externalEvents) {
			if (event != VK_NULL_HANDLE) {
				_gpuSystem->_eventDestroy(event);
			}
		}

		for (PassType srcPassType : EnumIter<PassType>::Iterates()) {
			for (PassType dstPassType : EnumIter<PassType>::Iterates()) {
				SemaphoreID semaphoreID = _externalSemaphores[srcPassType][dstPassType];
				if (semaphoreID != SEMAPHORE_ID_NULL) {
					_gpuSystem->_semaphoreDestroy(semaphoreID);
				}
			}
		}

		for (BufferExecInfo& bufferInfo : internalBufferInfos) {
			_gpuSystem->bufferDestroy(bufferInfo.bufferID);
		}

		for (TextureExecInfo& textureInfo : internalTextureInfos) {
			_gpuSystem->textureDestroy(textureInfo.textureID);
		}

		for (BufferExecInfo& bufferInfo : bufferInfos) {
			bufferInfo.passes.cleanup();
		}
		bufferInfos.cleanup();

		for (TextureExecInfo& textureInfo : textureInfos) {
			textureInfo.passes.cleanup();
		}
		textureInfos.cleanup();

		for (PassExecInfo& passInfo : passInfos) {
			passInfo.bufferInvalidates.cleanup();
			passInfo.bufferFlushes.cleanup();
			passInfo.textureInvalidates.cleanup();
			passInfo.textureFlushes.cleanup();
		}
		passInfos.cleanup();
	}

	void RenderGraphExecution::_initInShaderBuffers(const Array<ShaderBufferReadAccess>& shaderAccessList, int index, QueueFlagBits queueFlags) {
		PassExecInfo& passInfo = passInfos[index];
		for (const ShaderBufferReadAccess& shaderAccess : shaderAccessList) {
			SOUL_ASSERT(0, shaderAccess.nodeID != BUFFER_NODE_ID_NULL, "");

			uint32 bufferInfoID = getBufferInfoIndex(shaderAccess.nodeID);

			VkPipelineStageFlags stageFlags = vkCastShaderStageToPipelineStageFlags(shaderAccess.stageFlags);

			BufferBarrier invalidateBarrier;
			invalidateBarrier.stageFlags = stageFlags;
			invalidateBarrier.accessFlags = VK_ACCESS_SHADER_READ_BIT;
			invalidateBarrier.bufferInfoIdx = bufferInfoID;
			passInfo.bufferInvalidates.add(invalidateBarrier);

			BufferBarrier flushBarrier;
			flushBarrier.stageFlags = stageFlags;
			flushBarrier.accessFlags = 0;
			flushBarrier.bufferInfoIdx = bufferInfoID;
			passInfo.bufferFlushes.add(flushBarrier);

			updateBufferInfo(&bufferInfos[bufferInfoID], queueFlags, getBufferUsageFromShaderReadUsage(shaderAccess.usage), PassNodeID(index));
		}

	}

	void RenderGraphExecution::_initOutShaderBuffers(const Array<ShaderBufferWriteAccess>& shaderAccessList, int index, QueueFlagBits queueFlags) {
		PassExecInfo& passInfo = passInfos[index];
		for (const ShaderBufferWriteAccess& shaderAccess : shaderAccessList) {
			SOUL_ASSERT(0, shaderAccess.nodeID != BUFFER_NODE_ID_NULL, "");

			uint32 bufferInfoID = getBufferInfoIndex(shaderAccess.nodeID);

			VkPipelineStageFlags stageFlags = vkCastShaderStageToPipelineStageFlags(shaderAccess.stageFlags);

			BufferBarrier invalidateBarrier;
			invalidateBarrier.stageFlags = stageFlags;
			invalidateBarrier.accessFlags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			invalidateBarrier.bufferInfoIdx = bufferInfoID;
			passInfo.bufferInvalidates.add(invalidateBarrier);

			BufferBarrier flushBarrier;
			flushBarrier.stageFlags = stageFlags;
			flushBarrier.accessFlags = VK_ACCESS_SHADER_WRITE_BIT;
			flushBarrier.bufferInfoIdx = bufferInfoID;
			passInfo.bufferFlushes.add(flushBarrier);

			updateBufferInfo(&bufferInfos[bufferInfoID], queueFlags, getBufferUsageFromShaderWriteUsage(shaderAccess.usage), PassNodeID(index));
		}

	}

	void RenderGraphExecution::_initShaderTextures(const Array<ShaderTextureReadAccess>& shaderAccessList, int index, QueueFlagBits queueFlags) {
		PassExecInfo& passInfo = passInfos[index];
		for (const ShaderTextureReadAccess& shaderAccess : shaderAccessList) {

			SOUL_ASSERT(0, shaderAccess.nodeID != TEXTURE_NODE_ID_NULL, "");

			uint32 textureInfoID = getTextureInfoIndex(shaderAccess.nodeID);
			VkPipelineStageFlags stageFlags = vkCastShaderStageToPipelineStageFlags(shaderAccess.stageFlags);

			updateTextureInfo(&textureInfos[textureInfoID], queueFlags,
			                  getTextureUsageFromShaderReadUsage(shaderAccess.usage), PassNodeID(index));

			auto isWriteableUsage = [](ShaderTextureReadUsage usage) -> bool {
				bool mapping[] = {
					false,
					true
				};
				return mapping[uint64(usage)];
			};

			VkImageLayout imageLayout = isWriteableUsage(shaderAccess.usage) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			TextureBarrier invalidateBarrier;
			invalidateBarrier.stageFlags = stageFlags;
			invalidateBarrier.accessFlags = VK_ACCESS_SHADER_READ_BIT;
			invalidateBarrier.layout = imageLayout;
			invalidateBarrier.textureInfoIdx = getTextureInfoIndex(shaderAccess.nodeID);
			passInfo.textureInvalidates.add(invalidateBarrier);

			TextureBarrier flushBarrier;
			flushBarrier.stageFlags = stageFlags;
			flushBarrier.accessFlags = 0;
			flushBarrier.layout = imageLayout;
			flushBarrier.textureInfoIdx = getTextureInfoIndex(shaderAccess.nodeID);
			passInfo.textureFlushes.add(flushBarrier);
		}
	}

	void RenderGraphExecution::_initShaderTextures(const Array<ShaderTextureWriteAccess>& shaderAccessList, int index, QueueFlagBits queueFlags) {
		PassExecInfo& passInfo = passInfos[index];
		for (const ShaderTextureWriteAccess& shaderAccess : shaderAccessList) {
			SOUL_ASSERT(0, shaderAccess.nodeID != TEXTURE_NODE_ID_NULL, "");

			uint32 textureInfoID = getTextureInfoIndex(shaderAccess.nodeID);
			VkPipelineStageFlags stageFlags = vkCastShaderStageToPipelineStageFlags(shaderAccess.stageFlags);

			updateTextureInfo(&textureInfos[textureInfoID], queueFlags,
			                  getTextureUsageFromShaderWriteUsage(shaderAccess.usage), PassNodeID(index));

			//TODO(kevinyu) : check whether the barrier is correct

			TextureBarrier invalidateBarrier;
			invalidateBarrier.stageFlags = stageFlags;
			invalidateBarrier.accessFlags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			invalidateBarrier.layout = VK_IMAGE_LAYOUT_GENERAL;
			invalidateBarrier.textureInfoIdx = getTextureInfoIndex(shaderAccess.nodeID);
			passInfo.textureInvalidates.add(invalidateBarrier);

			TextureBarrier flushBarrier;
			flushBarrier.stageFlags = stageFlags;
			flushBarrier.accessFlags = VK_ACCESS_SHADER_WRITE_BIT;
			flushBarrier.layout = VK_IMAGE_LAYOUT_GENERAL;
			flushBarrier.textureInfoIdx = getTextureInfoIndex(shaderAccess.nodeID);
			passInfo.textureFlushes.add(flushBarrier);
		}
	}

}
