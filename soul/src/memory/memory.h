#pragma once

#include "core/type.h"
#include "core/dev_util.h"

namespace Soul {namespace Memory {

	namespace Util
	{
		static void* AlignForward(void* address, uint32 alignment)
		{
			return (void*)((uintptr(address) + alignment) & ~(alignment - 1));
		}
	}
	
	class Allocator
	{
	public:
		Allocator() = delete;
		Allocator(const char* name) { _name = name; }
		virtual ~Allocator() {};

		const char* getName() { return _name; }
		
		virtual void* allocate(uint32 size, uint32 alignment) = 0;
		virtual void deallocate(void* addr) = 0;

		template <typename TYPE, typename... ARGS>
		TYPE* make(ARGS&&... args)
		{
			void* address = allocate(sizeof(TYPE), alignof(TYPE));
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

		MallocAllocator(const char* name);
		virtual ~MallocAllocator();
		virtual void* allocate(uint32 size, uint32 alignment) final override;
		virtual void deallocate(void* addr) final override;
		
	};

	class LinearAllocator: public Allocator
	{
	public:

		LinearAllocator(const char* name, void* begin, uint32 size, Allocator* fallbackAllocator);
		virtual ~LinearAllocator();
		virtual void* allocate(uint32 size, uint32 alignment) final override;
		virtual void deallocate(void* addr) final override;

	private:
		void* startAddr;
		uint32 size;
		Allocator* allocator;

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
		
		ProxyAllocator() = default;
		
		template<typename... ARGS>
		ProxyAllocator(const char* name, ARGS&&... args) : Allocator(name), allocator(std::forward<ARGS>(args) ... )
		{
			
		}

		virtual void* allocate(uint32 size, uint32 alignment) final override
		{
			_threadingProxy.lock();

			void* address = allocator.allocate(size, alignment);
			_trackingProxy.onAllocate(address, size);
			_taggingProxy.onAllocate(address, size);
			_threadingProxy.unlock();
			return address;
		}

		virtual void deallocate(void* addr) final override
		{
			_threadingProxy.lock();
			_trackingProxy.onDeallocate(addr);
			_taggingProxy.onDeallocate(addr);
			allocator.deallocate(addr);
			_threadingProxy.unlock();
		}

	private:
		THREADING_PROXY _threadingProxy;
		TRACKING_PROXY _trackingProxy;
		TAGGING_PROXY _taggingProxy;
	};






	
} }
