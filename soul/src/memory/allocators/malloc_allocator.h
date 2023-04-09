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

    auto reset() -> void override;
    auto try_allocate(soul_size size, soul_size alignment, const char* tag) -> Allocation override;
    [[nodiscard]] auto get_allocation_size(void* addr) const -> soul_size override;
    auto deallocate(void* addr) -> void override;
  };

} // namespace soul::memory
