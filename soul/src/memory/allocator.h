#pragma once

#include "core/type.h"
#include "core/dev_util.h"

#include "memory/util.h"

#include <tuple>

#define ONE_KILOBYTE 1024
#define ONE_MEGABYTE 1024 * ONE_KILOBYTE
#define ONE_GIGABYTE 1024 * ONE_MEGABYTE

namespace Soul { namespace Memory {

	struct Allocation {
		void* addr;
		uint64 size;
	};

	class Allocator
	{
	public:
		Allocator() = delete;
		explicit Allocator(const char* name) { _name = name; }
		virtual ~Allocator() {};

		const char* getName() { return _name; }

		virtual void reset() = 0;
		virtual Allocation allocate(uint32 size, uint32 alignment, const char* tag) = 0;
		virtual void deallocate(void* addr, uint32 size) = 0;

		virtual void* allocate(uint32 size, uint32 alignment) final {
			return allocate(size, alignment, "untagged").addr;
		}

		template <typename TYPE, typename... ARGS>
		TYPE* create(ARGS&&... args)
		{
			Allocation allocation = allocate(sizeof(TYPE), alignof(TYPE), "untagged");
			return allocation.addr ? new(allocation.addr) TYPE(std::forward<ARGS>(args)...) : nullptr;
		}

		template <typename TYPE>
		void destroy(TYPE* ptr)
		{
			SOUL_ASSERT(0, ptr != nullptr, "");
			ptr->~TYPE();
			deallocate(ptr, sizeof(TYPE));
		}

	private:
		const char* _name = nullptr;
	};



}}