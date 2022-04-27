#pragma once

#include "core/type.h"
#include "core/dev_util.h"

namespace soul::memory {

	constexpr soul_size ONE_KILOBYTE = 1024;
	constexpr soul_size ONE_MEGABYTE = 1024 * ONE_KILOBYTE;
	constexpr soul_size ONE_GIGABYTE = 1024 * ONE_MEGABYTE;

	struct Allocation {
		void* addr = nullptr;
		soul_size size = 0;

		Allocation() = default;
		Allocation(void* addr, soul_size size): addr(addr), size(size) {}
	};

	class Allocator {
	public:
		Allocator() = delete;
		explicit Allocator(const char* name) noexcept : _name(name) {}
		Allocator(const Allocator& other) = delete;
		Allocator& operator=(const Allocator& other) = delete;
		Allocator(Allocator&& other) = delete;
		Allocator& operator=(Allocator&& other) = delete;
		virtual ~Allocator() = default;

		SOUL_NODISCARD const char* name() const { return _name; }

		
		virtual Allocation try_allocate(soul_size size, soul_size alignment, const char* tag) = 0;
		virtual void* allocate(soul_size size, soul_size alignment) final {
			const Allocation allocation = try_allocate(size, alignment, "untagged");
			return allocation.addr;
		}

		virtual void* allocate(soul_size size, soul_size alignment, const char* tag) final {
			const Allocation allocation = try_allocate(size, alignment, tag);
			return allocation.addr;
		}
		virtual void deallocate(void* addr, soul_size size) = 0;

		template <typename TYPE, typename... ARGS>
		TYPE* create(ARGS&&... args)
		{
			Allocation allocation = try_allocate(sizeof(TYPE), alignof(TYPE), "untagged");
			return allocation.addr ? new(allocation.addr) TYPE(std::forward<ARGS>(args)...) : nullptr;
		}

		template <typename TYPE>
		void destroy(TYPE* ptr)
		{
			SOUL_ASSERT(0, ptr != nullptr, "");
			ptr->~TYPE();
			deallocate(ptr, ptr->class_size());
		}

		virtual void reset() = 0;

	private:
		const char* _name = nullptr;
	};

}
