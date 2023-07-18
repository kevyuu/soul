#include "memory/allocators/malloc_allocator.h"

namespace soul::memory
{

  MallocAllocator::MallocAllocator(const char* name) : Allocator(name) {}

  auto MallocAllocator::reset() -> void { SOUL_NOT_IMPLEMENTED(); }

  auto MallocAllocator::try_allocate(usize size, usize /* alignment */, const char* /* tag */)
    -> Allocation
  {
    void* addr = malloc(size); // NOLINT
    return {addr, size};
  }

  auto MallocAllocator::get_allocation_size(void* addr) const -> usize
  {
    if (addr == nullptr) {
      return 0;
    }
    return _msize(addr);
  }

  void MallocAllocator::deallocate(void* addr)
  {
    free(addr); // NOLINT
  }

} // namespace soul::memory
