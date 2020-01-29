#pragma once

#include "core/type.h"
#include "core/dev_util.h"

namespace Soul { namespace Memory {

	class Allocator
	{
	public:
		Allocator() = delete;
		Allocator(const char* name) { _name = name; }
		virtual ~Allocator() {};

		const char* getName() { return _name; }

		virtual void init() = 0;
		virtual void cleanup() = 0;
		virtual void* allocate(uint32 size, uint32 alignment, const char* tag) = 0;
		virtual void deallocate(void* addr) = 0;

		template <typename TYPE, typename... ARGS>
		TYPE* make(ARGS&&... args)
		{
			void* address = allocate(sizeof(TYPE), alignof(TYPE), _name);
			return address ? new(address) TYPE(std::forward<ARGS>(args)...) : nullptr;
		}

		template <typename TYPE>
		void destroy(TYPE* ptr)
		{
			SOUL_ASSERT(0, ptr != nullptr, "");
			ptr->~TYPE();
			deallocate(ptr);
		}

	private:
		const char* _name = nullptr;
	};

	class MallocAllocator: public Allocator
	{
	public:

		MallocAllocator() = delete;
		MallocAllocator(const char* name);
		virtual ~MallocAllocator() = default;

		MallocAllocator(const MallocAllocator& other) = delete;
		MallocAllocator& operator=(const MallocAllocator& other) = delete;

		MallocAllocator(MallocAllocator&& other) = delete;
		MallocAllocator& operator=(MallocAllocator&& other) = delete;

		virtual void init() final override;
		virtual void cleanup() final override;
		virtual void* allocate(uint32 size, uint32 alignment, const char* tag = "") final override;
		virtual void deallocate(void* addr) final override;

	};

	namespace ThreadingProxy
	{
		struct NoSync
		{
			void lock() {}
			void unlock() {}
		};
	}

	namespace TrackingProxy
	{
		struct Untrack
		{
			void onAllocate(void* address, uint32 size) {}
			void onDeallocate(void* address) {}
		};

		struct CounterCheck
		{
			uint32 counter = 0;
			void onAllocate(void* address, uint32 size) { counter++; }
			void onDeallocate(void* address) { counter--; }
			~CounterCheck()
			{
				SOUL_ASSERT(0, counter == 0, "");
			}
		};
	}

	namespace TaggingProxy
	{
		struct Untagged
		{
			void onAllocate(void* address, uint32 size) {}
			void onDeallocate(void* address) {}
		};
	}

	template<
			typename BACKING_ALLOCATOR,
			typename THREADING_PROXY = ThreadingProxy::NoSync,
			typename TRACKING_PROXY = TrackingProxy::Untrack,
			typename TAGGING_PROXY = TaggingProxy::Untagged>
	class ProxyAllocator : public Allocator {
	public:
		BACKING_ALLOCATOR allocator;

		ProxyAllocator() = delete;
		template<typename... ARGS>
		ProxyAllocator(const char* name, ARGS&&... args) :
				Allocator(name), allocator(name, std::forward<ARGS>(args) ... ) {}
		virtual ~ProxyAllocator() = default;

		ProxyAllocator(const ProxyAllocator& other) = delete;
		ProxyAllocator& operator=(const ProxyAllocator& other) = delete;

		ProxyAllocator(ProxyAllocator&& other) = delete;
		ProxyAllocator& operator=(ProxyAllocator&& other) = delete;

		virtual void init() final override;
		virtual void cleanup() final override;

		virtual void* allocate(uint32 size, uint32 alignment, const char* tag = "") final override;

		virtual void deallocate(void* addr) final override;
	private:
		THREADING_PROXY _threadingProxy;
		TRACKING_PROXY _trackingProxy;
		TAGGING_PROXY _taggingProxy;
	};

}}