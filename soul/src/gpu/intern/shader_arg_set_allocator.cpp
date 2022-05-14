#include "shader_arg_set_allocator.h"
#include "../type.h"
#include "../system.h"

#include "enum_mapping.h"

namespace soul::gpu::impl
{

	void ShaderArgSetAllocator::DescriptorSetDeleter::operator()(DescriptorSet& descriptor_set)
	{
		if (!set_cache_->contains(descriptor_set.setLayout))
		{
			set_cache_->insert(descriptor_set.setLayout, Array<VkDescriptorSet>());
		}
		(*set_cache_)[descriptor_set.setLayout].push_back(descriptor_set.vkHandle);
	}

	void ShaderArgSetAllocator::init(System* gpu_system, VkDevice device)
	{
		gpu_system_ = gpu_system;
		device_ = device;
		thread_contexts_.init_construct(runtime::get_thread_count(), 
		[this, device](soul_size index, ThreadContext* buffer)
			{
				const VkDescriptorPoolSize pool_sizes[2] = {
				{
					.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
					.descriptorCount = 2000
				},
				{
					.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 4000
				}
				};
				const VkDescriptorPoolCreateInfo pool_info = {
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
					.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
					.maxSets = 2000,
					.poolSizeCount = std::size(pool_sizes),
					.pPoolSizes = pool_sizes
				};
				VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
				SOUL_VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool), "");
				if (descriptor_pool == VK_NULL_HANDLE) SOUL_LOG_WARN("Fail to create descriptor pool");
				new (buffer) ThreadContext(device, descriptor_pool, allocator_);
			});

	}

	ShaderArgSetAllocator::ThreadContext& ShaderArgSetAllocator::get_thread_context()
	{
		return thread_contexts_[runtime::get_thread_id()];
	}

	ShaderArgSetAllocator::DescriptorSetKey ShaderArgSetAllocator::get_descriptor_set_key(const ShaderArgSetDesc& arg_set_desc, uint32& offset_count, uint32* offsets)
	{
		uint64 hash = 0;
		std::for_each(arg_set_desc.bindingDescriptions, arg_set_desc.bindingDescriptions + arg_set_desc.bindingCount,
			[this, &offset_count, &hash, offsets](const Descriptor& desc)
		{

			switch (desc.type) {
			case DescriptorType::NONE:
			{
				hash = hash_fnv1(&desc.type, hash);
				break;
			}
			case DescriptorType::UNIFORM_BUFFER: {
				const UniformDescriptor& descriptor = desc.uniformInfo;
				hash = hash_fnv1(&desc.type, hash);
				hash = hash_fnv1(&descriptor.bufferID.id.cookie, hash);
				offsets[offset_count++] =
					descriptor.unitIndex * soul::cast<uint32>(gpu_system_->get_buffer_ptr(descriptor.bufferID)->unitSize);
				break;
			}
			case DescriptorType::SAMPLED_IMAGE: {
				hash = hash_fnv1(&desc.type, hash);
				const SampledImageDescriptor& descriptor = desc.sampledImageInfo;
				hash = hash_fnv1(&descriptor.textureID.id.cookie, hash);
				hash = hash_fnv1(&desc.sampledImageInfo.samplerID, hash);
				hash = hash_fnv1(&desc.sampledImageInfo.view, hash);
				break;
			}
			case DescriptorType::INPUT_ATTACHMENT: {
				hash = hash_fnv1(&desc.type, hash);
				hash = hash_fnv1(&(gpu_system_->get_texture_ptr(desc.inputAttachmentInfo.textureID)->view), hash);
				break;
			}
			case DescriptorType::STORAGE_IMAGE: {
				hash = hash_fnv1(&desc.type, hash);
				hash = hash_fnv1(&(gpu_system_->get_texture_ptr(desc.storageImageInfo.textureID)->view), hash);
				hash = hash_fnv1(&desc.storageImageInfo.mipLevel, hash);
				break;
			}
			default: {
				SOUL_NOT_IMPLEMENTED();
				break;
			}
			}
		});

		return hash;
	}

	ShaderArgSetAllocator::ShaderArgSetKey ShaderArgSetAllocator::get_shader_arg_set_key(uint32 offset_count, const uint32* offsets, VkDescriptorSet descriptor_set)
	{
		soul_size hash = 0;
		hash = hash_fnv1(&descriptor_set, hash);
		return hash_fnv1_bytes(cast<uint8*>(offsets), sizeof(uint32) * offset_count, hash);
	}

	ShaderArgSetAllocator::ID ShaderArgSetAllocator::request_shader_arg_set(const ShaderArgSetDesc& desc)
	{
		SOUL_PROFILE_ZONE();
		auto& thread_context = get_thread_context();
		ID result;
		DescriptorSetKey descriptor_set_key = get_descriptor_set_key(desc, result.offsetCount, result.offset);
		DescriptorSet* set = thread_context.descriptorSetCache.get_or_create(descriptor_set_key,
		[this](const ShaderArgSetDesc& desc, VkDevice device, VkDescriptorPool descriptor_pool, FreeDescriptorSetCache& free_set_cache)->DescriptorSet
			{
				DescriptorSetLayoutKey set_layout_key = {};
				for (uint32 binding_idx = 0; binding_idx < desc.bindingCount; binding_idx++) {
					const Descriptor& descriptor_desc = desc.bindingDescriptions[binding_idx];
					if (descriptor_desc.type == DescriptorType::NONE) continue;
					set_layout_key.bindings[binding_idx].descriptorCount = 1;
					set_layout_key.bindings[binding_idx].descriptorType = vkCast(descriptor_desc.type);
					set_layout_key.bindings[binding_idx].stageFlags = vkCast(descriptor_desc.stageFlags);
				}

				VkDescriptorSetLayout set_layout = gpu_system_->request_descriptor_layout(set_layout_key);
				const VkDescriptorSetAllocateInfo alloc_info = {
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
					.descriptorPool = descriptor_pool,
					.descriptorSetCount = 1,
					.pSetLayouts = &set_layout
				};

				VkDescriptorSet descriptor_set = [&]() -> VkDescriptorSet
				{
					if (!free_set_cache.contains(set_layout) || free_set_cache[set_layout].size() == 0)
					{
						VkDescriptorSet result;
						SOUL_VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, &result), "");
						return result;
					}
					else
					{
						VkDescriptorSet result = free_set_cache[set_layout].back();
						free_set_cache[set_layout].pop();
						return result;
					}
				}();
				
				if (descriptor_set == VK_NULL_HANDLE)
					SOUL_LOG_WARN("Descriptor set creation fail");

				for (uint32 binding_idx = 0; binding_idx < desc.bindingCount; binding_idx++) {
					const Descriptor& descriptor_desc = desc.bindingDescriptions[binding_idx];
					VkWriteDescriptorSet descriptor_write = {
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.dstSet = descriptor_set,
						.dstBinding = binding_idx,
						.dstArrayElement = 0,
						.descriptorCount = 1,
						.descriptorType = vkCast(descriptor_desc.type)
					};

					VkDescriptorBufferInfo buffer_info;
					VkDescriptorImageInfo image_info;
					switch (descriptor_desc.type) {
					case DescriptorType::NONE:
					{
						continue;
					}
					case DescriptorType::UNIFORM_BUFFER: {
						buffer_info.buffer = gpu_system_->get_buffer_ptr(descriptor_desc.uniformInfo.bufferID)->vkHandle;
						Buffer* buffer = gpu_system_->get_buffer_ptr(descriptor_desc.uniformInfo.bufferID);
						buffer_info.offset = 0;
						buffer_info.range = buffer->unitSize;
						descriptor_write.pBufferInfo = &buffer_info;
						break;
					}
					case DescriptorType::SAMPLED_IMAGE: {
						image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						TextureID texID = descriptor_desc.sampledImageInfo.textureID;
						image_info.imageView = gpu_system_->get_texture_view(texID, descriptor_desc.sampledImageInfo.view);
						image_info.sampler = descriptor_desc.sampledImageInfo.samplerID.id;
						descriptor_write.pImageInfo = &image_info;
						break;
					}
					case DescriptorType::INPUT_ATTACHMENT: {
						image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						image_info.imageView = gpu_system_->get_texture_ptr(descriptor_desc.inputAttachmentInfo.textureID)->view;
						image_info.sampler = VK_NULL_HANDLE;
						descriptor_write.pImageInfo = &image_info;
						break;
					}
					case DescriptorType::STORAGE_IMAGE: {
						image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
						image_info.imageView = gpu_system_->get_texture_view(descriptor_desc.storageImageInfo.textureID, descriptor_desc.storageImageInfo.mipLevel);
						image_info.sampler = VK_NULL_HANDLE;
						descriptor_write.pImageInfo = &image_info;
						break;
					}
					default: {
						SOUL_NOT_IMPLEMENTED();
						break;
					}
					}

					vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
				}
				return { descriptor_set, set_layout };
			}, desc, device_, thread_context.descriptorPool, thread_context.freeDescriptorSetCache);

		result.vkHandle = set->vkHandle;
		return result;
	}

	void ShaderArgSetAllocator::on_new_frame()
	{
		for (ThreadContext& thread_context : thread_contexts_)
		{
			thread_context.descriptorSetCache.on_new_frame();
		}
	}

	ShaderArgSetAllocator::~ShaderArgSetAllocator()
	{
		for (ThreadContext& thread_context : thread_contexts_)
		{
			vkResetDescriptorPool(device_, thread_context.descriptorPool, 0);
			vkDestroyDescriptorPool(device_, thread_context.descriptorPool, nullptr);
		}
	}

}