#include "bindless_descriptor_allocator.h"

#include "enum_mapping.h"
#include "gpu/system.h"

namespace soul::gpu::impl
{

	BindlessDescriptorAllocator::BindlessDescriptorAllocator(memory::Allocator* allocator) :
		storage_buffer_descriptor_set_(STORAGE_BUFFER_DESCRIPTOR_COUNT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, allocator),
		sampler_descriptor_set_(SAMPLER_DESCRIPTOR_COUNT, VK_DESCRIPTOR_TYPE_SAMPLER, allocator),
		sampled_image_descriptor_set_(SAMPLED_IMAGE_DESCRIPTOR_COUNT, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, allocator),
		storage_image_descriptor_set_(STORAGE_IMAGE_DESCRIPTOR_COUNT, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, allocator)
	{}

	BindlessDescriptorAllocator::~BindlessDescriptorAllocator()
	{
		if (device_ != VK_NULL_HANDLE && descriptor_pool_ != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
		else
			SOUL_ASSERT(0, device_ == VK_NULL_HANDLE && descriptor_pool_ == VK_NULL_HANDLE, "");
	}


	void BindlessDescriptorAllocator::init(VkDevice device)
	{
		device_ = device;
		const VkDescriptorPoolSize pool_sizes[] = {
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = STORAGE_BUFFER_DESCRIPTOR_COUNT
			},
			{
				.type = VK_DESCRIPTOR_TYPE_SAMPLER,
				.descriptorCount = SAMPLER_DESCRIPTOR_COUNT
			},
			{
				.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
				.descriptorCount = SAMPLED_IMAGE_DESCRIPTOR_COUNT
			},
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = STORAGE_IMAGE_DESCRIPTOR_COUNT
			}
		};
		const VkDescriptorPoolCreateInfo pool_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
			.maxSets = BINDLESS_SET_COUNT,
			.poolSizeCount = std::size(pool_sizes),
			.pPoolSizes = pool_sizes
		};
		SOUL_VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool_), "Fail to create descriptor pool");

		storage_buffer_descriptor_set_.init(device, descriptor_pool_);
		sampler_descriptor_set_.init(device, descriptor_pool_);
		sampled_image_descriptor_set_.init(device, descriptor_pool_);
		storage_image_descriptor_set_.init(device, descriptor_pool_);

		VkDescriptorSetLayout descriptor_set_layouts[BINDLESS_SET_COUNT] = {};
		descriptor_set_layouts[STORAGE_BUFFER_DESCRIPTOR_SET_INDEX] = storage_buffer_descriptor_set_.get_descriptor_set_layout();
		descriptor_set_layouts[SAMPLER_DESCRIPTOR_SET_INDEX] = sampler_descriptor_set_.get_descriptor_set_layout();
		descriptor_set_layouts[SAMPLED_IMAGE_DESCRIPTOR_SET_INDEX] = sampled_image_descriptor_set_.get_descriptor_set_layout();
		descriptor_set_layouts[STORAGE_IMAGE_DESCRIPTOR_SET_INDEX] = storage_image_descriptor_set_.get_descriptor_set_layout();

		VkPushConstantRange push_constant_range = {
			.stageFlags = VK_SHADER_STAGE_ALL,
			.offset = 0,
			.size = PUSH_CONSTANT_SIZE
		};

		const VkPipelineLayoutCreateInfo pipeline_layout_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = BINDLESS_SET_COUNT,
			.pSetLayouts = descriptor_set_layouts,
			.pushConstantRangeCount = 1,
			.pPushConstantRanges = &push_constant_range
		};

		SOUL_VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout_), "Fail to create pipeline layout");
	}

	DescriptorID BindlessDescriptorAllocator::create_storage_buffer_descriptor(VkBuffer buffer)
	{
		const VkDescriptorBufferInfo buffer_info = {
			.buffer = buffer,
			.offset = 0,
			.range = VK_WHOLE_SIZE
		};
		return storage_buffer_descriptor_set_.create_descriptor(device_, buffer_info);
	}

	void BindlessDescriptorAllocator::destroy_storage_buffer_descriptor(DescriptorID id)
	{
		storage_buffer_descriptor_set_.destroy_descriptor(device_, id);
	}

	DescriptorID BindlessDescriptorAllocator::create_sampled_image_descriptor(VkImageView image_view)
	{
		const VkDescriptorImageInfo image_info = {
			.sampler = VK_NULL_HANDLE,
			.imageView = image_view,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		return sampled_image_descriptor_set_.create_descriptor(device_, image_info);
	}

	void BindlessDescriptorAllocator::destroy_sampled_image_descriptor(DescriptorID id)
	{
		sampled_image_descriptor_set_.destroy_descriptor(device_, id);
	}

	DescriptorID BindlessDescriptorAllocator::create_storage_image_descriptor(VkImageView image_view)
	{
		const VkDescriptorImageInfo image_info = {
			.sampler = VK_NULL_HANDLE,
			.imageView = image_view,
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL
		};
		return storage_image_descriptor_set_.create_descriptor(device_, image_info);
	}

	void BindlessDescriptorAllocator::destroy_storage_image_descriptor(DescriptorID id)
	{
		storage_image_descriptor_set_.destroy_descriptor(device_, id);
	}

	DescriptorID BindlessDescriptorAllocator::create_sampler_descriptor(VkSampler sampler)
	{
		const VkDescriptorImageInfo image_info = {
			.sampler = sampler
		};
		return sampler_descriptor_set_.create_descriptor(device_, image_info);
	}

	void BindlessDescriptorAllocator::destroy_sampler_descriptor(DescriptorID id)
	{
		sampler_descriptor_set_.destroy_descriptor(device_, id);
	}

	BindlessDescriptorSet::BindlessDescriptorSet(uint32 capacity, VkDescriptorType descriptor_type, soul::memory::Allocator* allocator) : allocator_(allocator), free_head_(1), list_(allocator->allocate_array<uint32>(capacity)), capacity_(capacity), descriptor_type_(descriptor_type)
	{
		for (uint32 i = free_head_; i < capacity_; i++)
		{
			list_[i] = i + 1;
		}
	}

	void BindlessDescriptorSet::init(VkDevice device, VkDescriptorPool descriptor_pool)
	{
		const VkDescriptorSetLayoutBinding binding = {
			.binding = 0,
			.descriptorType = descriptor_type_,
			.descriptorCount = capacity_,
			.stageFlags = VK_SHADER_STAGE_ALL
		};

		const VkDescriptorBindingFlags flags =
			VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
			VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;

		const VkDescriptorSetLayoutBindingFlagsCreateInfo flag_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
			.bindingCount = 1,
			.pBindingFlags = &flags
		};

		const VkDescriptorSetLayoutCreateInfo set_layout_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = &flag_info,
			.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
			.bindingCount = 1,
			.pBindings = &binding
		};

		SOUL_VK_CHECK(vkCreateDescriptorSetLayout(device, &set_layout_info, nullptr, &descriptor_set_layout_), "Fail to create descriptor set layout");
		
		const VkDescriptorSetAllocateInfo set_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = descriptor_pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &descriptor_set_layout_
		};
		SOUL_VK_CHECK(vkAllocateDescriptorSets(device, &set_info, &descriptor_set_), "Fail to allocate descriptor sets");
	}

	DescriptorID BindlessDescriptorSet::create_descriptor(VkDevice device, const VkDescriptorBufferInfo& buffer_info)
	{
		std::unique_lock lock(mutex_);
		uint32 index = free_head_;
		free_head_ = list_[free_head_];
		const VkWriteDescriptorSet descriptor_write = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptor_set_,
			.dstBinding = 0,
			.dstArrayElement = index,
			.descriptorCount = 1,
			.descriptorType = descriptor_type_,
			.pBufferInfo = &buffer_info
		};
		vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
		return DescriptorID(index);
	}

	DescriptorID BindlessDescriptorSet::create_descriptor(VkDevice device, const VkDescriptorImageInfo& image_info)
	{
		std::unique_lock lock(mutex_);
		uint32 index = free_head_;
		free_head_ = list_[free_head_];
		const VkWriteDescriptorSet descriptor_write = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptor_set_,
			.dstBinding = 0,
			.dstArrayElement = index,
			.descriptorCount = 1,
			.descriptorType = descriptor_type_,
			.pImageInfo = &image_info
		};
		vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
		return DescriptorID(index);
	}

	void BindlessDescriptorSet::destroy_descriptor(VkDevice device, DescriptorID id)
	{
		if (id.is_null()) return;
		VkCopyDescriptorSet copy_descriptor_set = {
			.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
			.srcSet = descriptor_set_,
			.srcBinding = 0,
			.srcArrayElement = 0,
			.dstSet = descriptor_set_,
			.dstBinding = 0,
			.dstArrayElement = id.id,
			.descriptorCount = 1
		};
		vkUpdateDescriptorSets(device, 0, nullptr, 1, &copy_descriptor_set);
		list_[id.id] = free_head_;
		free_head_ = id.id;
	}

	BindlessDescriptorSet::~BindlessDescriptorSet()
	{
		allocator_->deallocate_array(list_, capacity_);
	}




}