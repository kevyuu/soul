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
		explicit Allocator(const char* name) noexcept : name_(name) {}
		Allocator(const Allocator& other) = delete;
		Allocator& operator=(const Allocator& other) = delete;
		Allocator(Allocator&& other) = delete;
		Allocator& operator=(Allocator&& other) = delete;
		virtual ~Allocator() = default;

		virtual Allocation try_allocate(soul_size size, soul_size alignment, const char* tag) = 0;
		virtual void deallocate(void* addr) = 0;
		virtual soul_size get_allocation_size(void* addr) const = 0;
		virtual void reset() = 0;

		[[nodiscard]] const char* name() const { return name_; }

		[[nodiscard]] void* allocate(const soul_size size, const soul_size alignment) {
			const Allocation allocation = try_allocate(size, alignment, "untagged");
			return allocation.addr;
		}

		[[nodiscard]] void* allocate(const soul_size size, const soul_size alignment, const char* tag) {
			const Allocation allocation = try_allocate(size, alignment, tag);
			return allocation.addr;
		}

		template<typename T>
		[[nodiscard]] T* allocate_array(const soul_size count, const char* tag = "untagged")
		{
			const Allocation allocation = try_allocate(count * sizeof(T), alignof(T), tag);  // NOLINT(bugprone-sizeof-expression)
			return static_cast<T*>(allocation.addr);
		}

		template<typename T>
		void deallocate_array(T* addr, const soul_size count)
		{
			deallocate(addr);  // NOLINT(bugprone-sizeof-expression)
		}

		template <typename Type, typename... Args>
		[[nodiscard]] Type* create(Args&&... args)
		{
			Allocation allocation = try_allocate(sizeof(Type), alignof(Type), "untagged");
			return allocation.addr ? new(allocation.addr) Type(std::forward<Args>(args)...) : nullptr;
		}

		template <typename Type>
		void destroy(Type* ptr)
		{
			SOUL_ASSERT(0, ptr != nullptr, "");
			if constexpr(!std::is_trivially_destructible_v<Type>)
			{
				ptr->~Type();
			}

			deallocate(ptr);
		}

	private:
		const char* name_ = nullptr;
	};

}
