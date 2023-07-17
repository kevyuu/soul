#pragma once

#include "core/vector.h"
#include "memory/allocator.h"
#include "runtime/runtime.h"

namespace soul::runtime
{

  template <typename BackingAllocator = TempAllocator>
  class ScopeAllocator final : public memory::Allocator
  {
  public:
    ScopeAllocator() = delete;

    explicit ScopeAllocator(
      const char* name,
      BackingAllocator* backing_allocator = get_temp_allocator(),
      Allocator* fallback_allocator = get_default_allocator()) noexcept;

    ScopeAllocator(const ScopeAllocator& other) = delete;

    auto operator=(const ScopeAllocator& other) -> ScopeAllocator& = delete;

    ScopeAllocator(ScopeAllocator&& other) = delete;

    auto operator=(ScopeAllocator&& other) -> ScopeAllocator& = delete;

    ~ScopeAllocator() override;

    void reset() override;

    auto try_allocate(usize size, usize alignment, const char* tag) -> memory::Allocation override;

    auto get_allocation_size(void* addr) const -> usize override;

    void deallocate(void* addr) override;

  private:
    BackingAllocator* backing_allocator_ = nullptr;
    void* scope_base_addr_ = nullptr;
    Allocator* fallback_allocator_ = nullptr;
    Vector<memory::Allocation> fallback_allocations_;

    [[nodiscard]]
    auto get_marker() const noexcept -> void*;

    void rewind(void* addr) noexcept;
  };

  template <typename BackingAllocator>
  ScopeAllocator<BackingAllocator>::ScopeAllocator(
    const char* name, BackingAllocator* backing_allocator, Allocator* fallback_allocator) noexcept
      : Allocator(name),
        backing_allocator_(backing_allocator),
        scope_base_addr_(backing_allocator_->get_marker()),
        fallback_allocator_(fallback_allocator),
        fallback_allocations_(fallback_allocator)
  {
  }

  template <typename BackingAllocator>
  ScopeAllocator<BackingAllocator>::~ScopeAllocator()
  {
    rewind(scope_base_addr_);
    for (const auto [addr, size] : fallback_allocations_) {
      fallback_allocator_->deallocate(addr);
    }
  }

  template <typename BackingAllocator>
  void ScopeAllocator<BackingAllocator>::reset()
  {
    rewind(scope_base_addr_);
  }

  template <typename BackingAllocator>
  auto ScopeAllocator<BackingAllocator>::try_allocate(usize size, usize alignment, const char* tag)
    -> memory::Allocation
  {
    memory::Allocation allocation = backing_allocator_->try_allocate(size, alignment, tag);
    if (allocation.addr == nullptr) {
      allocation = fallback_allocator_->try_allocate(size, alignment, tag);
      fallback_allocations_.push_back(allocation);
    }
    return allocation;
  }

  template <typename BackingAllocator>
  auto ScopeAllocator<BackingAllocator>::get_allocation_size(void* addr) const -> usize
  {
    return backing_allocator_->get_allocation_size(addr);
  }

  template <typename BackingAllocator>
  void ScopeAllocator<BackingAllocator>::deallocate(void* addr)
  {
  }

  template <typename BackingAllocator>
  auto ScopeAllocator<BackingAllocator>::get_marker() const noexcept -> void*
  {
    return backing_allocator_->getMarker();
  }

  template <typename BackingAllocator>
  void ScopeAllocator<BackingAllocator>::rewind(void* addr) noexcept
  {
    backing_allocator_->rewind(addr);
  }

} // namespace soul::runtime
