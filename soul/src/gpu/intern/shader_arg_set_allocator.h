#pragma once

#include "core/string.h"

#include <volk/volk.h>
#include "../object_cache.h"
#include "gpu/constant.h"

namespace soul::gpu
{
	class System;
	struct Descriptor;

	struct ShaderArgSetDesc {
		uint32 bindingCount;
		const Descriptor* bindingDescriptions;
	};
}

namespace soul::gpu::impl
{
	struct ShaderArgSet {
		VkDescriptorSet vkHandle = VK_NULL_HANDLE;
		uint32 offset[MAX_DYNAMIC_BUFFER_PER_SET] = {};
		uint32 offsetCount = 0;

		bool operator==(const ShaderArgSet& rhs) const noexcept {
			return vkHandle == rhs.vkHandle && !memcmp(offset, rhs.offset, sizeof(offset));
		}
	};

	class ShaderArgSetAllocator
	{
	public:
		using ID = ShaderArgSet;
		explicit ShaderArgSetAllocator(memory::Allocator* allocator = GetDefaultAllocator()) : allocator_(allocator) {}
		void init(System* gpu_system, VkDevice device);
		ID request_shader_arg_set(const ShaderArgSetDesc&);
		ShaderArgSet get(ID id) { return id; }
		void on_new_frame();

	private:
		static constexpr soul_size RING_SIZE = 12;
		
		using DescriptorSetKey = uint64;
		struct DescriptorSet
		{
			VkDescriptorSet vkHandle;
			VkDescriptorSetLayout setLayout;
		};
		using FreeDescriptorSetCache = HashMap<VkDescriptorSetLayout, Array<VkDescriptorSet>>;

		struct DescriptorSetDeleter
		{
		public:
			explicit DescriptorSetDeleter(VkDevice device, VkDescriptorPool descriptor_pool, FreeDescriptorSetCache* set_cache) :
				device_(device), descriptor_pool_(descriptor_pool), set_cache_(set_cache) {}
			void operator()(DescriptorSet& descriptor_set);
		private:
			VkDevice device_;
			VkDescriptorPool descriptor_pool_;
			FreeDescriptorSetCache* set_cache_;
		};

		using ShaderArgSetKey = uint64;

		struct ThreadContext
		{
			memory::ProxyAllocator<memory::Allocator, memory::ProfileProxy> proxyAllocator;
			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
			FreeDescriptorSetCache freeDescriptorSetCache;
			RingCache<DescriptorSetKey, DescriptorSet, RING_SIZE, DescriptorSetDeleter> descriptorSetCache;
			soul_size requestCount = 0;
			ThreadContext(VkDevice device, VkDescriptorPool descriptor_pool, memory::Allocator* allocator):
				proxyAllocator(memory::ProxyAllocator<memory::Allocator, memory::ProfileProxy>("Thread Context Allocator", allocator, memory::ProfileProxy({}))),
				descriptorPool(descriptor_pool),
				descriptorSetCache(&proxyAllocator, DescriptorSetDeleter(device, descriptor_pool, &freeDescriptorSetCache))
			{

				freeDescriptorSetCache.reserve(100);
			}
		};

		ThreadContext& get_thread_context();
		DescriptorSetKey get_descriptor_set_key(const ShaderArgSetDesc& arg_set_desc, uint32& offset_count, uint32* offsets);
		ShaderArgSetKey get_shader_arg_set_key(uint32 offset_count, const uint32* offsets, VkDescriptorSet descriptor_set);

		StaticArray<ThreadContext> thread_contexts_;
		memory::Allocator* allocator_ = nullptr;
		System* gpu_system_ = nullptr;
		VkDevice device_ = VK_NULL_HANDLE;
	};
}