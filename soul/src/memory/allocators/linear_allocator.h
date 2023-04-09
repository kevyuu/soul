#pragma once

#include "memory/allocator.h"

namespace soul::memory
{

  class LinearAllocator final : public Allocator
  {
  public:
    LinearAllocator() = delete;
    LinearAllocator(const char* name, soul_size size, Allocator* backing_allocator);
    LinearAllocator(const LinearAllocator& other) = delete;
    auto operator=(const LinearAllocator& other) -> LinearAllocator& = delete;
    LinearAllocator(LinearAllocator&& other) = delete;
    auto operator=(LinearAllocator&& other) -> LinearAllocator& = delete;
    ~LinearAllocator() override;

    auto reset() -> void override;
    auto try_allocate(soul_size size, soul_size alignment, const char* tag) -> Allocation override;
    auto get_allocation_size(void* addr) const -> soul_size override;
    auto deallocate(void* addr) -> void override;
    [[nodiscard]] auto get_marker() const noexcept -> void*;
    auto rewind(void* addr) noexcept -> void;

  private:
    Allocator* backing_allocator_;
    void* base_addr_ = nullptr;
    void* current_addr_ = nullptr;
    uint64 size_ = 0;
  };

} // namespace soul::memory
