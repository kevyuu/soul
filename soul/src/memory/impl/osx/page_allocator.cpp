#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

#include "memory/allocators/page_allocator.h"
#include "memory/util.h"

namespace soul::memory
{

  PageAllocator::PageAllocator(CompStr name) : Allocator(name)
  {
    page_size_ = getpagesize();
  }

  auto PageAllocator::reset() -> void{};

  auto PageAllocator::try_allocate(uint32 size, uint32 alignment, StringView tag) -> Allocation
  {
    uint32 new_size = util::pointer_page_size_round(size, page_size_);
    void* addr = mmap(nullptr, new_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    return {addr, new_size};
  }

  auto PageAllocator::deallocate(void* addr, uint32 size) -> void
  {
    SOUL_ASSERT(0, util::pointer_page_size_round(size, page_size_) == size);
    munmap(addr, size);
  }

} // namespace soul::memory
