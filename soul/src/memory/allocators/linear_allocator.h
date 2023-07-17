#pragma once

#include "memory/allocator.h"

namespace soul::memory
{

  class LinearAllocator final : public Allocator
  {
  public:
    LinearAllocator() = delete;

    LinearAllocator(const char* name, usize size, Allocator* backing_allocator);

    LinearAllocator(const LinearAllocator& other) = delete;

    auto operator=(const LinearAllocator& other) -> LinearAllocator& = delete;

    LinearAllocator(LinearAllocator&& other) = delete;

    auto operator=(LinearAllocator&& other) -> LinearAllocator& = delete;

    ~LinearAllocator() override;

    void reset() override;

    auto try_allocate(usize size, usize alignment, const char* tag) -> Allocation override;

    auto get_allocation_size(void* addr) const -> usize override;

    void deallocate(void* addr) override;

    [[nodiscard]]
    auto get_marker() const -> void*;

    void rewind(void* addr);

  private:
    Allocator* backing_allocator_;
    void* base_addr_ = nullptr;
    void* current_addr_ = nullptr;
    u64 size_ = 0;
  };

} // namespace soul::memory
