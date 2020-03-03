#include "core/util.h"

#include "job/system.h"

#include "gpu/system.h"
#include "gpu/render_graph.h"
#include "gpu/render_graph_registry.h"
#include "gpu/intern/render_graph_execution.h"
#include "gpu/intern/enum_mapping.h"

#include "memory/allocators/scope_allocator.h"
#include "memory/allocators/linear_allocator.h"

#include <volk/volk.h>

namespace Soul {namespace GPU {

	static EnumArray<PassType, QueueType> _PASS_TYPE_TO_QUEUE_TYPE_MAP({
		QueueType::NONE,
		QueueType::GRAPHIC,
		QueueType::COMPUTE,
		QueueType::TRANSFER,
	});

	static EnumArray<ResourceOwner, PassType> _RESOURCE_OWNER_TO_PASS_TYPE_MAP({
		PassType::NONE,
		PassType::GRAPHIC,
		PassType::COMPUTE,
		PassType::TRANSFER,
		PassType::NONE
	});

	void _RenderGraphExecution::init() {
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
			static auto getBufferUsageFlagsFromDescriptorType = [](DescriptorType type) -> BufferUsageFlags {
				SOUL_ASSERT(0, DescriptorTypeUtil::IsBuffer(type), "");
				static constexpr BufferUsageFlags mapping[] = {
					BUFFER_USAGE_UNIFORM_BIT,
					0
				};
				static_assert(SOUL_ARRAY_LEN(mapping) == uint64(DescriptorType::COUNT) , "");
				return mapping[uint64(type)];
			};

			static auto getTextureUsageFlagsFromDescriptorType = [](DescriptorType type) -> TextureUsageFlags {
				SOUL_ASSERT(0, DescriptorTypeUtil::IsTexture(type), "");
				static constexpr TextureUsageFlags mapping[] = {
					0,
					TEXTURE_USAGE_SAMPLED_BIT
				};
				static_assert(SOUL_ARRAY_LEN(mapping) == uint64(DescriptorType::COUNT), "");
				return mapping[uint64(type)];
			};

			static auto updateBufferInfo = [](_RGBufferExecInfo* bufferInfo, QueueFlagBits queueFlag,
											  VkFlags usageFlags, PassNodeID passID) {

				bufferInfo->usageFlags |= usageFlags;
				bufferInfo->queueFlags |= queueFlag;
				if (bufferInfo->firstPass == PASS_NODE_ID_NULL) {
					bufferInfo->firstPass = passID;
				}
				bufferInfo->lastPass = passID;
				bufferInfo->passes.add(passID);
			};

			static auto updateTextureInfo = [](_RGTextureExecInfo* textureInfo, QueueFlagBits queueFlag,
											   VkFlags usageFlags, PassNodeID passID) {

				textureInfo->usageFlags |= usageFlags;
				textureInfo->queueFlags |= queueFlag;
				if (textureInfo->firstPass == PASS_NODE_ID_NULL) {
					textureInfo->firstPass = passID;
				}
				textureInfo->lastPass = passID;
				textureInfo->passes.add(passID);
			};

			const PassNode& passNode = *_renderGraph->_passNodes[i];
			_RGExecPassInfo& passInfo = passInfos[i];

			switch (passNode.type) {
				case PassType::NONE: {
					break;
				}
				case PassType::GRAPHIC: {

					auto graphicNode = (const GraphicBaseNode *) &passNode;
					passInfo.programID = _gpuSystem->programRequest(*graphicNode);
					const _Program& program = *_gpuSystem->_programPtr(passInfo.programID);

					for (const BufferNodeID nodeID : graphicNode->vertexBuffers) {
						SOUL_ASSERT(0, nodeID != BUFFER_NODE_ID_NULL, "");

						uint32 bufferInfoID = getBufferInfoIndex(nodeID);

						passInfo.bufferInvalidates.add(_BufferBarrier());
						_BufferBarrier& invalidateBarrier = passInfo.bufferInvalidates.back();
						invalidateBarrier.stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
						invalidateBarrier.accessFlags = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
						invalidateBarrier.bufferInfoIdx = bufferInfoID;


						passInfo.bufferFlushes.add(_BufferBarrier());
						_BufferBarrier& flushBarrier = passInfo.bufferFlushes.back();
						flushBarrier.stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
						flushBarrier.accessFlags = 0;
						flushBarrier.bufferInfoIdx = bufferInfoID;

						updateBufferInfo(&bufferInfos[bufferInfoID], QUEUE_GRAPHIC_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, PassNodeID(i));
					}

					for (const BufferNodeID nodeID : graphicNode->indexBuffers) {
						SOUL_ASSERT(0, nodeID != BUFFER_NODE_ID_NULL, "");

						uint32 bufferInfoID = getBufferInfoIndex(nodeID);

						passInfo.bufferInvalidates.add(_BufferBarrier());
						_BufferBarrier& invalidateBarrier = passInfo.bufferInvalidates.back();
						invalidateBarrier.stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
						invalidateBarrier.accessFlags = VK_ACCESS_INDEX_READ_BIT;
						invalidateBarrier.bufferInfoIdx = bufferInfoID;

						passInfo.bufferFlushes.add(_BufferBarrier());
						_BufferBarrier& flushBarrier = passInfo.bufferFlushes.back();
						flushBarrier.stageFlags = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
						flushBarrier.accessFlags = 0;
						flushBarrier.bufferInfoIdx = bufferInfoID;

						updateBufferInfo(&bufferInfos[bufferInfoID], QUEUE_GRAPHIC_BIT, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, PassNodeID(i));
					}

					for (const ShaderBuffer& shaderBuffer : graphicNode->inShaderBuffers) {
						SOUL_ASSERT(0, shaderBuffer.nodeID != BUFFER_NODE_ID_NULL, "");
						SOUL_ASSERT(0, shaderBuffer.set < MAX_SET_PER_SHADER_PROGRAM, "");
						SOUL_ASSERT(0, shaderBuffer.binding < MAX_BINDING_PER_SET, "");
						const _ProgramDescriptorBinding& binding = program.bindings[shaderBuffer.set][shaderBuffer.binding];
						SOUL_ASSERT(0, binding.shaderStageFlags != 0,
								"No binding for set = %d, binding = %d detected on the shaders.",
								shaderBuffer.set, shaderBuffer.binding);
						SOUL_ASSERT(0, DescriptorTypeUtil::IsBuffer(binding.type), "");

						uint32 bufferInfoID = getBufferInfoIndex(shaderBuffer.nodeID);

						_BufferBarrier invalidateBarrier;
						invalidateBarrier.stageFlags = binding.pipelineStageFlags;
						invalidateBarrier.accessFlags = VK_ACCESS_SHADER_READ_BIT;
						invalidateBarrier.bufferInfoIdx = bufferInfoID;
						passInfo.bufferInvalidates.add(invalidateBarrier);

						_BufferBarrier flushBarrier;
						flushBarrier.stageFlags = binding.pipelineStageFlags;
						flushBarrier.accessFlags = 0;
						flushBarrier.bufferInfoIdx = bufferInfoID;
						passInfo.bufferFlushes.add(flushBarrier);

						updateBufferInfo(&bufferInfos[bufferInfoID], QUEUE_GRAPHIC_BIT, getBufferUsageFlagsFromDescriptorType(binding.type), passNodeID);

					}

					for (const ShaderBuffer& shaderBuffer : graphicNode->outShaderBuffers) {
						SOUL_ASSERT(0, shaderBuffer.nodeID != BUFFER_NODE_ID_NULL, "");
						SOUL_ASSERT(0, shaderBuffer.set < MAX_SET_PER_SHADER_PROGRAM, "");
						SOUL_ASSERT(0, shaderBuffer.binding < MAX_BINDING_PER_SET, "");
						const _ProgramDescriptorBinding& binding = program.bindings[shaderBuffer.set][shaderBuffer.binding];
						SOUL_ASSERT(0, binding.shaderStageFlags != 0,
								"No binding for set = %d, binding = %d detected on the shaders.",
								shaderBuffer.set, shaderBuffer.binding);
						SOUL_ASSERT(0, DescriptorTypeUtil::IsWriteableBuffer(binding.type), "");

						uint32 bufferInfoID = getBufferInfoIndex(shaderBuffer.nodeID);

						_BufferBarrier invalidateBarrier;
						invalidateBarrier.stageFlags = binding.pipelineStageFlags;
						invalidateBarrier.accessFlags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
						invalidateBarrier.bufferInfoIdx = getBufferInfoIndex(shaderBuffer.nodeID);
						passInfo.bufferInvalidates.add(invalidateBarrier);

						_BufferBarrier flushBarrier;
						flushBarrier.stageFlags = binding.pipelineStageFlags;
						flushBarrier.accessFlags = VK_ACCESS_SHADER_WRITE_BIT;
						flushBarrier.bufferInfoIdx = getBufferInfoIndex(shaderBuffer.nodeID);
						passInfo.bufferFlushes.add(flushBarrier);

						updateBufferInfo(&bufferInfos[bufferInfoID], QUEUE_GRAPHIC_BIT, getBufferUsageFlagsFromDescriptorType(binding.type), passNodeID);
					}

					for (const ShaderTexture& shaderTexture : graphicNode->inShaderTextures) {
						SOUL_ASSERT(0, shaderTexture.nodeID != TEXTURE_NODE_ID_NULL, "");
						SOUL_ASSERT(0, shaderTexture.set < MAX_SET_PER_SHADER_PROGRAM, "");
						SOUL_ASSERT(0, shaderTexture.binding < MAX_BINDING_PER_SET, "");
						const _ProgramDescriptorBinding& binding = program.bindings[shaderTexture.set][shaderTexture.binding];
						SOUL_ASSERT(0, binding.shaderStageFlags != 0,
								"No binding for set = %d, binding = %d detected on the shaders.",
								shaderTexture.set, shaderTexture.binding);
						SOUL_ASSERT(0, DescriptorTypeUtil::IsTexture(binding.type),
								"Cannot bind texture to set = %d, binding = %d.",
								shaderTexture.set, shaderTexture.binding);

						uint32 textureInfoID = getTextureInfoIndex(shaderTexture.nodeID);
						updateTextureInfo(&textureInfos[textureInfoID], QUEUE_GRAPHIC_BIT,
										  getTextureUsageFlagsFromDescriptorType(binding.type), passNodeID);

						VkImageLayout layout = DescriptorTypeUtil::IsWriteableTexture(binding.type) ?
								VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

						_TextureBarrier invalidateBarrier;
						invalidateBarrier.stageFlags = binding.pipelineStageFlags;
						invalidateBarrier.accessFlags = VK_ACCESS_SHADER_READ_BIT;
						invalidateBarrier.layout = layout;
						invalidateBarrier.textureInfoIdx = getTextureInfoIndex(shaderTexture.nodeID);
						passInfo.textureInvalidates.add(invalidateBarrier);

						_TextureBarrier flushBarrier;
						flushBarrier.stageFlags = binding.pipelineStageFlags;
						flushBarrier.accessFlags = 0;
						flushBarrier.layout = layout;
						flushBarrier.textureInfoIdx = getTextureInfoIndex(shaderTexture.nodeID);
						passInfo.textureFlushes.add(flushBarrier);
					}

					for (const ShaderTexture& shaderTexture : graphicNode->outShaderTextures) {
						SOUL_ASSERT(0, shaderTexture.nodeID != TEXTURE_NODE_ID_NULL, "");
						SOUL_ASSERT(0, shaderTexture.set < MAX_SET_PER_SHADER_PROGRAM, "");
						SOUL_ASSERT(0, shaderTexture.binding < MAX_BINDING_PER_SET, "");
						const _ProgramDescriptorBinding& binding = program.bindings[shaderTexture.set][shaderTexture.binding];
						SOUL_ASSERT(0, binding.shaderStageFlags != 0,
								"No binding for set = %d, binding = %d detected on the shaders.",
								shaderTexture.set, shaderTexture.binding);
						SOUL_ASSERT(0, DescriptorTypeUtil::IsWriteableTexture(binding.type), "");

						uint32 textureInfoID = getTextureInfoIndex(shaderTexture.nodeID);
						updateTextureInfo(&textureInfos[textureInfoID], QUEUE_GRAPHIC_BIT,
										  getTextureUsageFlagsFromDescriptorType(binding.type), passNodeID);

						_TextureBarrier invalidateBarrier;
						invalidateBarrier.stageFlags = binding.pipelineStageFlags;
						invalidateBarrier.accessFlags = VK_ACCESS_SHADER_READ_BIT;
						invalidateBarrier.layout = VK_IMAGE_LAYOUT_GENERAL;
						invalidateBarrier.textureInfoIdx = getTextureInfoIndex(shaderTexture.nodeID);
						passInfo.textureInvalidates.add(invalidateBarrier);

						_TextureBarrier flushBarrier;
						flushBarrier.stageFlags = binding.pipelineStageFlags;
						flushBarrier.accessFlags = 0;
						flushBarrier.layout = VK_IMAGE_LAYOUT_GENERAL;
						flushBarrier.textureInfoIdx = getTextureInfoIndex(shaderTexture.nodeID);
						passInfo.textureFlushes.add(flushBarrier);
					}

					for (const ColorAttachment& colorAttachment : graphicNode->colorAttachments) {
						SOUL_ASSERT(0, colorAttachment.nodeID != TEXTURE_NODE_ID_NULL, "");

						uint32 textureInfoID = getTextureInfoIndex(colorAttachment.nodeID);
						updateTextureInfo(&textureInfos[textureInfoID], QUEUE_GRAPHIC_BIT,
										  TEXTURE_USAGE_COLOR_ATTACHMENT_BIT, passNodeID);

						_TextureBarrier invalidateBarrier;
						invalidateBarrier.stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
						invalidateBarrier.accessFlags = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
						invalidateBarrier.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
						invalidateBarrier.textureInfoIdx = getTextureInfoIndex(colorAttachment.nodeID);
						passInfo.textureInvalidates.add(invalidateBarrier);

						_TextureBarrier flushBarrier;
						flushBarrier.stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
						flushBarrier.accessFlags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
						flushBarrier.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
						flushBarrier.textureInfoIdx = getTextureInfoIndex(colorAttachment.nodeID);
						passInfo.textureFlushes.add(flushBarrier);
					}

					if (graphicNode->depthStencilAttachment.nodeID != TEXTURE_NODE_ID_NULL) {
						uint32 resourceInfoIndex = getTextureInfoIndex(graphicNode->depthStencilAttachment.nodeID);

						uint32 textureInfoID = getTextureInfoIndex(graphicNode->depthStencilAttachment.nodeID);
						updateTextureInfo(&textureInfos[textureInfoID], QUEUE_GRAPHIC_BIT,
										  TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, passNodeID);

						_TextureBarrier invalidateBarrier;
						invalidateBarrier.stageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
						invalidateBarrier.accessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
						invalidateBarrier.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
						invalidateBarrier.textureInfoIdx = resourceInfoIndex;
						passInfo.textureInvalidates.add(invalidateBarrier);

						_TextureBarrier flushBarrier;
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

		for (VkEvent& event : _externalEvents) {
			event = VK_NULL_HANDLE;
		}

		for (auto srcPassType : Enum<PassType>::Iterates()) {
			for (auto dstPassType : Enum<PassType>::Iterates()) {
				_externalSemaphores[srcPassType][dstPassType] = SEMAPHORE_ID_NULL;
			}
		}

		for (int i = 0; i < _renderGraph->_internalBuffers.size(); i++) {
			const _RGInternalBuffer& rgBuffer = _renderGraph->_internalBuffers[i];
			_RGBufferExecInfo& bufferInfo = bufferInfos[i];

			BufferDesc desc;
			desc.typeSize = rgBuffer.typeSize;
			desc.typeAlignment = rgBuffer.typeAlignment;
			desc.count = rgBuffer.count;
			desc.queueFlags = bufferInfo.queueFlags;
			desc.usageFlags = bufferInfo.usageFlags;

			bufferInfo.bufferID = _gpuSystem->bufferCreate(desc);
		}

		for (int i = 0; i < externalBufferInfos.size(); i++) {
			_RGBufferExecInfo& bufferInfo = externalBufferInfos[i];
			if (bufferInfo.passes.size() == 0) continue;

			bufferInfo.bufferID = _renderGraph->_externalBuffers[i].bufferID;

			PassType firstPassType = _renderGraph->_passNodes[bufferInfo.passes[0].id]->type;
			ResourceOwner owner = _gpuSystem->_bufferPtr(bufferInfo.bufferID)->owner;
			PassType externalPassType = _RESOURCE_OWNER_TO_PASS_TYPE_MAP[owner];
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
				SemaphoreID& externalSemaphoreID = _externalSemaphores[externalPassType][firstPassType];
				if (externalSemaphoreID == SEMAPHORE_ID_NULL) {
					externalSemaphoreID = _gpuSystem->_semaphoreCreate();
				}
				bufferInfo.pendingEvent = VK_NULL_HANDLE;
				bufferInfo.pendingSemaphore = externalSemaphoreID;
				bufferInfo.unsyncWriteStage = 0;
				bufferInfo.unsyncWriteAccess = 0;
			}

		}

		for (int i = 0; i < _renderGraph->_internalTextures.size(); i++) {
			const _RGInternalTexture& rgTexture = _renderGraph->_internalTextures[i];
			_RGTextureExecInfo& textureInfo = textureInfos[i];

			TextureDesc desc;
			desc.width = rgTexture.width;
			desc.height = rgTexture.height;
			desc.depth = rgTexture.depth;
			desc.format = rgTexture.format;
			desc.queueFlags = textureInfo.queueFlags;
			desc.usageFlags = textureInfo.usageFlags;
			desc.mipLevels = rgTexture.mipLevels;
			desc.type = rgTexture.type;
			textureInfo.textureID = _gpuSystem->textureCreate(desc);
		}

		for (int i = 0; i < externalTextureInfos.size(); i++) {
			_RGTextureExecInfo& textureInfo = externalTextureInfos[i];
			if (textureInfo.passes.size() == 0) continue;
			textureInfo.textureID = _renderGraph->_externalTextures[i].textureID;

			PassType firstPassType = _renderGraph->_passNodes[textureInfo.passes[0].id]->type;
			ResourceOwner owner = _gpuSystem->_texturePtr(textureInfo.textureID)->owner;
			PassType externalPassType = _RESOURCE_OWNER_TO_PASS_TYPE_MAP[owner];
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

	VkRenderPass _RenderGraphExecution::_renderPassCreate(uint32 passIndex) {
		SOUL_ASSERT_MAIN_THREAD();

		auto graphicNode = (const GraphicBaseNode*) _renderGraph->_passNodes[passIndex];

		VkAttachmentDescription attachments[MAX_COLOR_ATTACHMENT_PER_SHADER] = {};

		for (int i = 0; i < graphicNode->colorAttachments.size(); i++) {
			const ColorAttachment& attachment = graphicNode->colorAttachments[i];
			uint32 textureInfoIdx = getTextureInfoIndex(attachment.nodeID);
			const _RGTextureExecInfo& textureInfo = textureInfos[textureInfoIdx];
			const _Texture& texture = *_gpuSystem->_texturePtr(textureInfo.textureID);

			attachments[i].format = vkCast(texture.format);
			attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;

			if (attachment.desc.clear) {
				attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			} else {
				attachments[i].loadOp = textureInfo.firstPass.id == passIndex && !isExternal(textureInfo) ?
						VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_LOAD;
			}
			attachments[i].storeOp = textureInfo.lastPass.id == passIndex && !isExternal(textureInfo) ?
					VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;

			attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			attachments[i].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachments[i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}

		VkAttachmentReference attachmentRefs[MAX_COLOR_ATTACHMENT_PER_SHADER + 1];
		for (int i = 0; i < graphicNode->colorAttachments.size(); i++) {
			attachmentRefs[i].attachment = i;
			attachmentRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}

		int attachmentCount = graphicNode->colorAttachments.size();

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = graphicNode->colorAttachments.size();
		subpass.pColorAttachments = attachmentRefs;
		if (graphicNode->depthStencilAttachment.nodeID != TEXTURE_NODE_ID_NULL) {
			const DepthStencilAttachment& attachment = graphicNode->depthStencilAttachment;

			VkAttachmentDescription& attachmentDesc = attachments[attachmentCount];
			uint32 textureInfoIdx = getTextureInfoIndex(graphicNode->depthStencilAttachment.nodeID);
			const _RGTextureExecInfo& textureInfo = textureInfos[textureInfoIdx];
			const _Texture& texture = *_gpuSystem->_texturePtr(textureInfo.textureID);
			attachmentDesc.flags = 0;
			attachmentDesc.format = vkCast(texture.format);
			attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;

			if (attachment.desc.clear) {
				attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			}
			else if (isExternal(textureInfo) || textureInfo.firstPass.id != passIndex) {
				attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			} else {
				attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			}

			attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			if (isExternal(textureInfo) || textureInfo.lastPass.id != passIndex) {
				attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			}

			attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

			attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference& depthAttachmentRef = attachmentRefs[attachmentCount];
			depthAttachmentRef.attachment = attachmentCount;
			depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			subpass.pDepthStencilAttachment = &depthAttachmentRef;

			attachmentCount++;
		}

		VkRenderPassCreateInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
		renderPassInfo.attachmentCount = attachmentCount;
		renderPassInfo.pAttachments = attachments;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 0;
		renderPassInfo.pDependencies = nullptr;

		return _gpuSystem->_renderPassCreate(renderPassInfo);
	}

	VkFramebuffer _RenderGraphExecution::_framebufferCreate(uint32 passIndex, VkRenderPass renderPass) {

		SOUL_ASSERT_MAIN_THREAD();

		auto graphicNode = (const GraphicBaseNode*) _renderGraph->_passNodes[passIndex];

		VkImageView imageViews[MAX_COLOR_ATTACHMENT_PER_SHADER + 1];
		int imageViewCount = 0;
		for (const ColorAttachment& attachment : graphicNode->colorAttachments) {
			const _Texture* texture = getTexture(attachment.nodeID);
			imageViews[imageViewCount++] = texture->view;
		}

		if (graphicNode->depthStencilAttachment.nodeID != TEXTURE_NODE_ID_NULL) {
			const _Texture* texture = getTexture(graphicNode->depthStencilAttachment.nodeID);
			imageViews[imageViewCount++] = texture->view;
		}

		VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = imageViewCount;
		framebufferInfo.pAttachments = imageViews;
		framebufferInfo.width = graphicNode->pipelineConfig.framebuffer.width;
		framebufferInfo.height = graphicNode->pipelineConfig.framebuffer.height;
		framebufferInfo.layers = 1;

		return _gpuSystem->_framebufferCreate(framebufferInfo);
	}



	void _RenderGraphExecution::_submitExternalSyncPrimitive() {

		// Sync semaphores
		for(PassType srcPassType : Enum<PassType>::Iterates()) {
			SemaphoreID semaphoreIDs[uint64(PassType::COUNT)];
			uint32 semaphoreCount = 0;

			QueueType srcQueueType = _PASS_TYPE_TO_QUEUE_TYPE_MAP[srcPassType];

			for (PassType dstPassType : Enum<PassType>::Iterates()) {
				if (_externalSemaphores[srcPassType][dstPassType] != SEMAPHORE_ID_NULL) {
					semaphoreIDs[semaphoreCount++] = _externalSemaphores[srcPassType][dstPassType];
				}
			}
			if (semaphoreCount != 0) {
				VkCommandBuffer syncSemaphoreCmdBuffer = _gpuSystem->_queueRequestCommandBuffer(srcQueueType);
				_gpuSystem->_queueSubmitCommandBuffer(srcQueueType, syncSemaphoreCmdBuffer, semaphoreCount, semaphoreIDs);
			}
		}

		// Sync events
		for(PassType passType : Enum<PassType>::Iterates()) {
			QueueType queueType = _PASS_TYPE_TO_QUEUE_TYPE_MAP[passType];
			if (_externalEvents[passType] != VK_NULL_HANDLE && passType != PassType::NONE) {
				VkCommandBuffer syncEventCmdBuffer = _gpuSystem->_queueRequestCommandBuffer(queueType);
				vkCmdSetEvent(syncEventCmdBuffer, _externalEvents[passType], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
				_gpuSystem->_queueSubmitCommandBuffer(queueType, syncEventCmdBuffer, 0, nullptr);
			}
		}
	}

	void _RenderGraphExecution::_executePass(uint32 passIndex, VkCommandBuffer cmdBuffer) {
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
			case PassType::COMPUTE:
				SOUL_NOT_IMPLEMENTED();
				break;
			case PassType::GRAPHIC: {
				auto graphicNode = (const GraphicBaseNode*) passNode;
				VkRenderPass renderPass = _renderPassCreate(passIndex);

				VkFramebuffer framebuffer = _framebufferCreate(passIndex, renderPass);

				VkRenderPassBeginInfo renderPassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
				renderPassBeginInfo.renderPass = renderPass;
				renderPassBeginInfo.framebuffer = framebuffer;
				renderPassBeginInfo.renderArea.offset = {0, 0};
				renderPassBeginInfo.renderArea.extent = {
						graphicNode->pipelineConfig.framebuffer.width,
						graphicNode->pipelineConfig.framebuffer.height
				};


				VkClearValue clearValues[MAX_COLOR_ATTACHMENT_PER_SHADER + 1] = {};
				uint32 clearCount = 0;

				for (ColorAttachment attachment : graphicNode->colorAttachments) {
					if (attachment.desc.clear) {
						ClearValue clearValue = attachment.desc.clearValue;
						clearValues[clearCount++] = {clearValue.color.x, clearValue.color.y, clearValue.color.z, clearValue.color.w };
					}
				}

				if (graphicNode->depthStencilAttachment.nodeID != TEXTURE_NODE_ID_NULL) {
					const DepthStencilAttachmentDesc& desc = graphicNode->depthStencilAttachment.desc;
					if (desc.clear) {
						ClearValue clearValue = desc.clearValue;
						clearValues[clearCount++] = {clearValue.depthStencil.x, clearValue.depthStencil.y };
					}
				}

				renderPassBeginInfo.clearValueCount = clearCount;
				renderPassBeginInfo.pClearValues = clearValues;

				vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				ProgramID programID = _gpuSystem->programRequest(*graphicNode);
				VkPipeline pipeline = _gpuSystem->_pipelineCreate(*graphicNode, programID, renderPass);
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

				Memory::ScopeAllocator<> scopeAllocator("command buckets");
				CommandBucket commandBucket(&scopeAllocator);
				RenderGraphRegistry registry(_gpuSystem, this, programID);
				passNode->executePass(&registry, &commandBucket);

				{
					SOUL_PROFILE_ZONE_WITH_NAME("Commands translation");
					for (int i = 0; i < commandBucket.commands.size(); i++) {
						Command::Command* command = commandBucket.commands[i];
						command->_submit(&_gpuSystem->_db, programID, cmdBuffer);
					}
				}
				vkCmdEndRenderPass(cmdBuffer);

				_gpuSystem->_pipelineDestroy(pipeline);
				_gpuSystem->_framebufferDestroy(framebuffer);
				_gpuSystem->_renderPassDestroy(renderPass);

				break;
			}
			default: {
				SOUL_NOT_IMPLEMENTED();
				break;
			}
		}
	}

	void _RenderGraphExecution::run() {
		SOUL_ASSERT_MAIN_THREAD();
		SOUL_PROFILE_ZONE();
		Memory::ScopeAllocator<> scopeAllocator("_RenderGraphExecution::run");

		_submitExternalSyncPrimitive();

		Array<VkEvent> garbageEvents(&scopeAllocator);
		Array<SemaphoreID> garbageSemaphores(&scopeAllocator);

		Array<VkBufferMemoryBarrier> eventBufferBarriers(&scopeAllocator);
		Array<VkImageMemoryBarrier> eventImageBarriers(&scopeAllocator);
		Array<VkImageMemoryBarrier> initLayoutBarriers(&scopeAllocator);
		Array<VkImageMemoryBarrier> semaphoreLayoutBarriers(&scopeAllocator);
		Array<VkEvent> events(&scopeAllocator);

		static auto needInvalidate = [](VkAccessFlags visibleAccessMatrix[],
			VkPipelineStageFlags stageFlags, VkAccessFlags accessFlags) -> bool {
			bool result = false;
			Util::ForEachBit(stageFlags, [&result, accessFlags, visibleAccessMatrix](uint32 bit) {
				if (accessFlags & ~visibleAccessMatrix[bit]) {
					result = true;
				}
			});
			return result;
		};

		for (int i = 0; i < _renderGraph->_passNodes.size(); i++) {
			PassNode* passNode = _renderGraph->_passNodes[i];
			QueueType queueType = _PASS_TYPE_TO_QUEUE_TYPE_MAP[passNode->type];
			_RGExecPassInfo& passInfo = passInfos[i];

			VkCommandBuffer cmdBuffer = _gpuSystem->_queueRequestCommandBuffer(queueType);

			eventBufferBarriers.resize(0);
			eventImageBarriers.resize(0);
			initLayoutBarriers.resize(0);
			semaphoreLayoutBarriers.resize(0);
			events.resize(0);

			VkPipelineStageFlags eventSrcStageFlags = 0;
			VkPipelineStageFlags eventDstStageFlags = 0;
			VkPipelineStageFlags semaphoreDstStageFlags = 0;
			VkPipelineStageFlags initLayoutDstStageFlags = 0;

			for (const _BufferBarrier& barrier: passInfo.bufferInvalidates) {
				_RGBufferExecInfo& bufferInfo = bufferInfos[barrier.bufferInfoIdx];

				if (bufferInfo.pendingSemaphore == SEMAPHORE_ID_NULL &&
					bufferInfo.unsyncWriteAccess == 0 &&
					!needInvalidate(bufferInfo.visibleAccessMatrix, bufferInfo.unsyncWriteAccess, bufferInfo.unsyncWriteStage)) {
					continue;
				}

				if (bufferInfo.pendingEvent != VK_NULL_HANDLE) {
					VkAccessFlags dstAccessFlags = barrier.accessFlags;

					VkBufferMemoryBarrier memBarrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
					memBarrier.srcAccessMask = bufferInfo.unsyncWriteAccess;
					memBarrier.dstAccessMask = dstAccessFlags;
					memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					memBarrier.buffer = _gpuSystem->_bufferPtr(bufferInfo.bufferID)->vkHandle;
					memBarrier.offset = 0;
					memBarrier.size = VK_WHOLE_SIZE;

					eventBufferBarriers.add(memBarrier);
					events.add(bufferInfo.pendingEvent);
					eventSrcStageFlags |= bufferInfo.unsyncWriteStage;
					eventDstStageFlags |= barrier.stageFlags;

					Util::ForEachBit(barrier.stageFlags, [&bufferInfo, dstAccessFlags](uint32 bit) {
						bufferInfo.visibleAccessMatrix[bit] |= dstAccessFlags;
					});

					bufferInfo.pendingEvent = VK_NULL_HANDLE;

				} else {

					SOUL_ASSERT(0, bufferInfo.pendingSemaphore != SEMAPHORE_ID_NULL, "");
					_Semaphore& semaphore = *_gpuSystem->_semaphorePtr(bufferInfo.pendingSemaphore);

					_gpuSystem->_queueWaitSemaphore(queueType, bufferInfo.pendingSemaphore, barrier.stageFlags);

					bufferInfo.pendingSemaphore = SEMAPHORE_ID_NULL;
				}
			}

			for (const _TextureBarrier& barrier: passInfo.textureInvalidates) {
				_RGTextureExecInfo& textureInfo = textureInfos[barrier.textureInfoIdx];
				_Texture* texture = _gpuSystem->_texturePtr(textureInfo.textureID);

				bool layoutChange = texture->layout != barrier.layout;

				if (textureInfo.pendingSemaphore == SEMAPHORE_ID_NULL &&
					!layoutChange &&
					textureInfo.unsyncWriteAccess == 0 &&
					!needInvalidate(textureInfo.visibleAccessMatrix, textureInfo.unsyncWriteAccess, textureInfo.unsyncWriteStage)) {
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
					_gpuSystem->_queueWaitSemaphore(QueueType::GRAPHIC, textureInfo.pendingSemaphore,
													barrier.stageFlags);
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
			for (const _BufferBarrier& barrier : passInfo.bufferFlushes) {
				_RGBufferExecInfo& bufferInfo = bufferInfos[barrier.bufferInfoIdx];
				if (bufferInfo.passCounter != bufferInfo.passes.size() - 1) {
					uint32 nextPassIdx = bufferInfo.passes[bufferInfo.passCounter + 1].id;
					PassType passType = _renderGraph->_passNodes[nextPassIdx]->type;
					isPassTypeDependent[passType] = true;
				}
			}

			for (const _TextureBarrier& barrier: passInfo.textureFlushes) {
				_RGTextureExecInfo& textureInfo = textureInfos[barrier.textureInfoIdx];
				if (textureInfo.passCounter != textureInfo.passes.size() - 1) {
					uint32 nextPassIdx = textureInfo.passes[textureInfo.passCounter + 1].id;
					PassType passType = _renderGraph->_passNodes[nextPassIdx]->type;
					isPassTypeDependent[passType] = true;
				}
			}

			EnumArray<PassType, SemaphoreID> semaphoresMap(SEMAPHORE_ID_NULL);
			VkEvent event = VK_NULL_HANDLE;
			VkPipelineStageFlags eventStageFlags = 0;

			for (PassType passType : Enum<PassType>::Iterates()) {
				if (isPassTypeDependent[passType] && passType == passNode->type) {
					event = _gpuSystem->_eventCreate();
					garbageEvents.add(event);
				} else if (isPassTypeDependent[passType] && passType != passNode->type) {
					semaphoresMap[passType] = _gpuSystem->_semaphoreCreate();
					garbageSemaphores.add(semaphoresMap[passType]);
				}
			}

			for (const _BufferBarrier& barrier : passInfo.bufferFlushes) {
				_RGBufferExecInfo& bufferInfo = bufferInfos[barrier.bufferInfoIdx];
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

			for (const _TextureBarrier& barrier: passInfo.textureFlushes) {
				_RGTextureExecInfo& textureInfo = textureInfos[barrier.textureInfoIdx];
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

			SemaphoreID semaphores[MAX_SIGNAL_SEMAPHORE];
			uint32 semaphoreCount = 0;
			for (SemaphoreID semaphoreID : semaphoresMap) {
				if (semaphoreID != SEMAPHORE_ID_NULL) {
					semaphores[semaphoreCount++] = semaphoreID;
				}
			}

			// Update unsync stage
			for (const _BufferBarrier& barrier : passInfo.bufferFlushes) {
				_RGBufferExecInfo& bufferInfo = bufferInfos[barrier.bufferInfoIdx];
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

			for (const _TextureBarrier& barrier: passInfo.textureFlushes) {
				_RGTextureExecInfo& textureInfo = textureInfos[barrier.textureInfoIdx];
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

			_gpuSystem->_queueSubmitCommandBuffer(queueType, cmdBuffer, semaphoreCount, semaphores);
		}

		// Update resource owner for external resource
		static EnumArray<PassType, ResourceOwner> PASS_TYPE_TO_RESOURCE_OWNER({
			ResourceOwner::NONE,
			ResourceOwner::GRAPHIC_QUEUE,
			ResourceOwner::COMPUTE_QUEUE,
			ResourceOwner::TRANSFER_QUEUE
		});

		for (const _RGTextureExecInfo& textureInfo : textureInfos) {
			if (textureInfo.passes.size() == 0) continue;
			_Texture* texture = _gpuSystem->_texturePtr(textureInfo.textureID);
			uint64 lastPassIdx = textureInfo.passes.back().id;
			PassType lastPassType = _renderGraph->_passNodes[lastPassIdx]->type;
			texture->owner = PASS_TYPE_TO_RESOURCE_OWNER[lastPassType];
		}

		for (const _RGBufferExecInfo& bufferInfo : bufferInfos) {
			if (bufferInfo.passes.size() == 0) continue;
			_Buffer* buffer = _gpuSystem->_bufferPtr(bufferInfo.bufferID);
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

	bool _RenderGraphExecution::isExternal(const _RGBufferExecInfo& info) const {
		return (&info - bufferInfos.data()) > _renderGraph->_internalBuffers.size();
	}

	bool _RenderGraphExecution::isExternal(const _RGTextureExecInfo& info) const {
		return (&info - textureInfos.data()) > _renderGraph->_internalTextures.size();
	}

	BufferID _RenderGraphExecution::getBufferID(BufferNodeID nodeID) const {
		uint32 infoIdx = getBufferInfoIndex(nodeID);
		const _RGBufferExecInfo& bufferInfo = bufferInfos[infoIdx];
		return bufferInfo.bufferID;
	}

	TextureID _RenderGraphExecution::getTextureID(TextureNodeID nodeID) const {
		uint32 infoIdx = getTextureInfoIndex(nodeID);
		const _RGTextureExecInfo& textureInfo = textureInfos[infoIdx];
		return textureInfo.textureID;
	}

	_Buffer* _RenderGraphExecution::getBuffer(BufferNodeID nodeID) const {
		uint32 infoIdx = getBufferInfoIndex(nodeID);
		const _RGBufferExecInfo& bufferInfo = bufferInfos[infoIdx];
		return _gpuSystem->_bufferPtr(bufferInfo.bufferID);
	}

	_Texture* _RenderGraphExecution::getTexture(TextureNodeID nodeID) const {
		uint32 infoIdx = getTextureInfoIndex(nodeID);
		const _RGTextureExecInfo& textureInfo = textureInfos[infoIdx];
		return _gpuSystem->_texturePtr(textureInfo.textureID);
	}

	uint32 _RenderGraphExecution::getBufferInfoIndex(BufferNodeID nodeID) const {
		const _BufferNode* node = _renderGraph->_bufferNodePtr(nodeID);
		if (node->resourceID.isExternal()) {
			return _renderGraph->_internalBuffers.size() + node->resourceID.getIndex();
		}
		return node->resourceID.getIndex();
	}

	uint32 _RenderGraphExecution::getTextureInfoIndex(TextureNodeID nodeID) const {
		const _TextureNode* node = _renderGraph->_textureNodePtr(nodeID);
		if (node->resourceID.isExternal()) {
			return _renderGraph->_internalTextures.size() + node->resourceID.getIndex();
		}
		return node->resourceID.getIndex();
	}

	void _RenderGraphExecution::cleanup() {

		for (VkEvent event : _externalEvents) {
			if (event != VK_NULL_HANDLE) {
				_gpuSystem->_eventDestroy(event);
			}
		}

		for (PassType srcPassType : Enum<PassType>::Iterates()) {
			for (PassType dstPassType : Enum<PassType>::Iterates()) {
				SemaphoreID semaphoreID = _externalSemaphores[srcPassType][dstPassType];
				if (semaphoreID != SEMAPHORE_ID_NULL) {
					_gpuSystem->_semaphoreDestroy(semaphoreID);
				}
			}
		}

		for (_RGBufferExecInfo& bufferInfo : internalBufferInfos) {
			_gpuSystem->bufferDestroy(bufferInfo.bufferID);
		}

		for (_RGTextureExecInfo& textureInfo : internalTextureInfos) {
			_gpuSystem->textureDestroy(textureInfo.textureID);
		}

		for (_RGBufferExecInfo& bufferInfo : bufferInfos) {
			bufferInfo.passes.cleanup();
		}
		bufferInfos.cleanup();

		for (_RGTextureExecInfo& textureInfo : textureInfos) {
			textureInfo.passes.cleanup();
		}
		textureInfos.cleanup();

		for (_RGExecPassInfo& passInfo : passInfos) {
			passInfo.bufferInvalidates.cleanup();
			passInfo.bufferFlushes.cleanup();
			passInfo.textureInvalidates.cleanup();
			passInfo.textureFlushes.cleanup();
		}
		passInfos.cleanup();
	}

}}