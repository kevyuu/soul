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
    auto try_allocate(soul_size size, soul_size alignment, const char* tag) -> Allocation override;
    [[nodiscard]] auto get_allocation_size(void* addr) const -> soul_size override;
    auto deallocate(void* addr) -> void override;

  private:
    soul_size page_size_ = 0;
  };

} // namespace soul::memory
