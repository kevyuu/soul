#pragma once

#include "memory/allocator.h"

namespace soul::memory
{

  class PageAllocator final : public Allocator
  {
  public:
    PageAllocator() = delete;
    explicit PageAllocator(const char* name);
    PageAllocator(const PageAllocator& other) = delete;
    auto operator=(const PageAllocator& other) -> PageAllocator& = delete;
    PageAllocator(PageAllocator&& other) = delete;
    auto operator=(PageAllocator&& other) -> PageAllocator& = delete;
    ~PageAllocator() override = default;

    auto reset() -> void override;
    auto try_allocate(usize size, usize alignment, const char* tag) -> Allocation override;
    [[nodiscard]]
    auto get_allocation_size(void* addr) const -> usize override;
    auto deallocate(void* addr) -> void override;

  private:
    usize page_size_ = 0;
  };

} // namespace soul::memory
