#pragma once

#include <volk/volk.h>
#include "core/type.h"
#include "core/mutex.h"
#include "gpu/constant.h"
#include "runtime/runtime.h"
#include "memory/allocator.h"

namespace soul::gpu
{
	class System;
	struct Descriptor;
	using DescriptorID = ID<Descriptor, uint32>;
}

namespace soul::gpu::impl
{
	struct BindlessDescriptorSets
	{
		VkDescriptorSet vkHandles[BINDLESS_SET_COUNT];
	};

	class BindlessDescriptorSet
	{
	public:
		explicit BindlessDescriptorSet(uint32 capacity, VkDescriptorType descriptor_type, soul::memory::Allocator* allocator = runtime::get_context_allocator());
		void init(VkDevice device, VkDescriptorPool descriptor_pool);
		DescriptorID create_descriptor(VkDevice device, const VkDescriptorBufferInfo& buffer_info);
		DescriptorID create_descriptor(VkDevice device, const VkDescriptorImageInfo& image_info);
		void destroy_descriptor(VkDevice device, DescriptorID id);
		VkDescriptorSet get_descriptor_set() const { return descriptor_set_; }
		VkDescriptorSetLayout get_descriptor_set_layout() const { return descriptor_set_layout_; }
		~BindlessDescriptorSet();
	private:
		
		soul::memory::Allocator* allocator_;
		uint32 free_head_;
		uint32* list_;
		uint32 capacity_;
		VkDescriptorType descriptor_type_;
		VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
		RWSpinMutex mutex_;
	};

	class BindlessDescriptorAllocator
	{
	public:
		explicit BindlessDescriptorAllocator(memory::Allocator* allocator = runtime::get_context_allocator());
		~BindlessDescriptorAllocator();
		void init(VkDevice device);

		DescriptorID create_storage_buffer_descriptor(VkBuffer buffer);
		void destroy_storage_buffer_descriptor(DescriptorID id);

		DescriptorID create_sampled_image_descriptor(VkImageView image_view);
		void destroy_sampled_image_descriptor(DescriptorID id);

		DescriptorID create_storage_image_descriptor(VkImageView image_view);
		void destroy_storage_image_descriptor(DescriptorID id);

		DescriptorID create_sampler_descriptor(VkSampler sampler);
		void destroy_sampler_descriptor(DescriptorID id);

		VkPipelineLayout get_pipeline_layout() const { return pipeline_layout_; }

		BindlessDescriptorSets get_bindless_descriptor_sets() const
		{
			return {
				storage_buffer_descriptor_set_.get_descriptor_set(),
				sampler_descriptor_set_.get_descriptor_set(),
				sampled_image_descriptor_set_.get_descriptor_set(),
				storage_image_descriptor_set_.get_descriptor_set()
			};
		}

	private:
		static constexpr uint32 STORAGE_BUFFER_DESCRIPTOR_COUNT = 512u * 1024;
		static constexpr uint32 SAMPLER_DESCRIPTOR_COUNT = 4u * 1024;
		static constexpr uint32 SAMPLED_IMAGE_DESCRIPTOR_COUNT = 512u * 1024;
		static constexpr uint32 STORAGE_IMAGE_DESCRIPTOR_COUNT = 512u * 1024;
		
		VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
		BindlessDescriptorSet storage_buffer_descriptor_set_;
		BindlessDescriptorSet sampler_descriptor_set_;
		BindlessDescriptorSet sampled_image_descriptor_set_;
		BindlessDescriptorSet storage_image_descriptor_set_;
		VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
		VkDevice device_ = VK_NULL_HANDLE;
	};
}