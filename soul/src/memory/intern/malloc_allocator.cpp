#include "memory/allocators/malloc_allocator.h"

namespace soul::memory
{

  MallocAllocator::MallocAllocator(const char* name) noexcept : Allocator(name) {}

  auto MallocAllocator::reset() -> void { SOUL_NOT_IMPLEMENTED(); }

  auto MallocAllocator::try_allocate(soul_size size, soul_size alignment, const char* tag)
    -> Allocation
  {
    void* addr = malloc(size);
    return {addr, size};
  }

  auto MallocAllocator::get_allocation_size(void* addr) const -> soul_size
  {
    if (addr == nullptr) {
      return 0;
    }
    return _msize(addr);
  }

  auto MallocAllocator::deallocate(void* addr) -> void { free(addr); }

} // namespace soul::memory
