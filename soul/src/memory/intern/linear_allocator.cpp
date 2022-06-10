#include "memory/allocators/linear_allocator.h"
#include "memory/util.h"

namespace soul::memory {

	LinearAllocator::LinearAllocator(const char* name, const soul_size size, Allocator* backing_allocator): Allocator(name), backing_allocator_(backing_allocator) {
		SOUL_ASSERT(0, backing_allocator_ != nullptr, "");
		const Allocation allocation = backing_allocator_->try_allocate(size, 0, name);
		base_addr_ = allocation.addr;
		current_addr_ = base_addr_;
		size_ = allocation.size;
	}

	LinearAllocator::~LinearAllocator() {
		backing_allocator_->deallocate(base_addr_);
	}

	void LinearAllocator::reset() { current_addr_ = base_addr_; }

	Allocation LinearAllocator::try_allocate(const soul_size size, const soul_size alignment, const char* tag) {
		auto* const size_addr = soul::cast<soul_size*>(util::pointer_align_forward(current_addr_, alignof(soul_size)));
		void* const addr = util::pointer_align_forward(util::pointer_add(size_addr, size), alignment);
		if (util::pointer_add(base_addr_, size_) < util::pointer_add(addr, size)) {
			return {nullptr, 0};
		}
		current_addr_ = util::pointer_add(addr, size);
		*size_addr = size;
		return {addr, size};
	}

	soul_size LinearAllocator::get_allocation_size(void* addr) const
	{
		if (addr == nullptr) return 0;
		void* size_addr = util::pointer_align_backward(util::pointer_sub(addr, sizeof(soul_size)), alignof(soul_size));
		return *soul::cast<soul_size*>(size_addr);
	}

	void LinearAllocator::deallocate(void* addr) {}

	void* LinearAllocator::get_marker() const noexcept {return current_addr_;}
	void LinearAllocator::rewind(void* addr) noexcept
	{
		SOUL_ASSERT(0, addr >= base_addr_ && addr <= current_addr_, "");
		current_addr_ = addr;
	}

}
