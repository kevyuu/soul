#include <Windows.h>

#include "memory/allocators/page_allocator.h"
#include "memory/util.h"

namespace soul::memory
{

  PageAllocator::PageAllocator(CompStr name) : Allocator(name)
  {
    SYSTEM_INFO s_sys_info;
    GetSystemInfo(&s_sys_info);
    page_size_ = s_sys_info.dwPageSize;
  }

  void PageAllocator::reset(){};

  auto PageAllocator::try_allocate(usize size, usize /* alignment */, StringView /* tag */)
    -> Allocation
  {
    auto new_size = util::pointer_page_size_round(size, page_size_);
    void* addr    = VirtualAlloc(nullptr, new_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    SOUL_ASSERT(0, addr != nullptr);
    return {addr, new_size};
  }

  auto PageAllocator::get_allocation_size(void* addr) const -> usize
  {
    if (addr == nullptr)
    {
      return 0;
    }
    MEMORY_BASIC_INFORMATION memory_basic_information;
    VirtualQuery(addr, &memory_basic_information, sizeof(memory_basic_information));
    return memory_basic_information.RegionSize;
  }

  void PageAllocator::deallocate(void* addr)
  {
    const auto is_success = VirtualFree(addr, 0, MEM_RELEASE);
    SOUL_ASSERT(0, is_success);
  }

} // namespace soul::memory
