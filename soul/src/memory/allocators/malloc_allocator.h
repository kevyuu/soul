#pragma once

#include "memory/allocator.h"

namespace soul::memory
{

  class MallocAllocator final : public Allocator
  {
  public:
    MallocAllocator() = delete;

    MallocAllocator(const char* name) noexcept;

    MallocAllocator(const MallocAllocator& other) = delete;

    auto operator=(const MallocAllocator& other) -> MallocAllocator& = delete;

    MallocAllocator(MallocAllocator&& other) = delete;

    auto operator=(MallocAllocator&& other) -> MallocAllocator& = delete;

    ~MallocAllocator() override = default;

    void reset() override;

    auto try_allocate(usize size, usize alignment, const char* tag) -> Allocation override;

    [[nodiscard]]
    auto get_allocation_size(void* addr) const -> usize override;

    void deallocate(void* addr) override;
  };

} // namespace soul::memory
