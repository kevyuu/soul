#include "memory/allocators/linear_allocator.h"
#include "memory/util.h"

namespace soul::memory
{

  LinearAllocator::LinearAllocator(
    const char* name, const usize size, Allocator* backing_allocator)
      : Allocator(name), backing_allocator_(backing_allocator)
  {
    SOUL_ASSERT(0, backing_allocator_ != nullptr, "");
    const Allocation allocation = backing_allocator_->try_allocate(size, 0, name);
    base_addr_ = allocation.addr;
    current_addr_ = base_addr_;
    size_ = allocation.size;
  }

  LinearAllocator::~LinearAllocator() { backing_allocator_->deallocate(base_addr_); }

  auto LinearAllocator::reset() -> void { current_addr_ = base_addr_; }

  auto LinearAllocator::try_allocate(
    const usize size, const usize alignment, const char* tag) -> Allocation
  {
    auto* const size_addr =
      soul::cast<usize*>(util::pointer_align_forward(current_addr_, alignof(usize)));
    void* const addr = util::pointer_align_forward(util::pointer_add(size_addr, size), alignment);
    if (util::pointer_add(base_addr_, size_) < util::pointer_add(addr, size)) {
      return {nullptr, 0};
    }
    current_addr_ = util::pointer_add(addr, size);
    *size_addr = size;
    return {addr, size};
  }

  auto LinearAllocator::get_allocation_size(void* addr) const -> usize
  {
    if (addr == nullptr) {
      return 0;
    }
    void* size_addr =
      util::pointer_align_backward(util::pointer_sub(addr, sizeof(usize)), alignof(usize));
    return *soul::cast<usize*>(size_addr);
  }

  auto LinearAllocator::deallocate(void* addr) -> void {}

  auto LinearAllocator::get_marker() const noexcept -> void* { return current_addr_; }

  auto LinearAllocator::rewind(void* addr) noexcept -> void
  {
    SOUL_ASSERT(0, addr >= base_addr_ && addr <= current_addr_, "");
    current_addr_ = addr;
  }

} // namespace soul::memory
