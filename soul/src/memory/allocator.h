#pragma once

#include "core/panic.h"
#include "core/type.h"

namespace soul::memory
{

  struct Allocation {
    void* addr = nullptr;
    usize size = 0;

    Allocation() = default;

    Allocation(void* addr, usize size) : addr(addr), size(size) {}
  };

  class Allocator
  {
  public:
    Allocator() = delete;

    explicit Allocator(const char* name) noexcept : name_(name) {}

    Allocator(const Allocator& other) = delete;
    auto operator=(const Allocator& other) -> Allocator& = delete;
    Allocator(Allocator&& other) = delete;
    auto operator=(Allocator&& other) -> Allocator& = delete;
    virtual ~Allocator() = default;

    virtual auto try_allocate(usize size, usize alignment, const char* tag) -> Allocation = 0;
    virtual auto deallocate(void* addr) -> void = 0;
    virtual auto get_allocation_size(void* addr) const -> usize = 0;
    virtual auto reset() -> void = 0;

    [[nodiscard]]
    auto name() const -> const char*
    {
      return name_;
    }

    [[nodiscard]]
    auto allocate(const usize size, const usize alignment) -> void*
    {
      const Allocation allocation = try_allocate(size, alignment, "untagged");
      return allocation.addr;
    }

    [[nodiscard]]
    auto allocate(const usize size, const usize alignment, const char* tag) -> void*
    {
      const Allocation allocation = try_allocate(size, alignment, tag);
      return allocation.addr;
    }

    template <typename T>
    [[nodiscard]]
    auto allocate_array(const usize count, const char* tag = "untagged") -> T*
    {
      const Allocation allocation = try_allocate(count * sizeof(T), alignof(T), tag);
      // NOLINT(bugprone-sizeof-expression)
      return static_cast<T*>(allocation.addr);
    }

    template <typename T>
    auto deallocate_array(T* addr, const usize count) -> void
    {
      deallocate(addr); // NOLINT(bugprone-sizeof-expression)
    }

    template <typename Type, typename... Args>
    [[nodiscard]]
    auto create(Args&&... args) -> Type*
    {
      Allocation allocation = try_allocate(sizeof(Type), alignof(Type), "untagged");
      return allocation.addr ? new (allocation.addr) Type(std::forward<Args>(args)...) : nullptr;
    }

    template <typename Type>
    auto destroy(Type* ptr) -> void
    {
      SOUL_ASSERT(0, ptr != nullptr, "");
      if constexpr (!std::is_trivially_destructible_v<Type>) {
        ptr->~Type();
      }

      deallocate(ptr);
    }

  private:
    const char* name_ = nullptr;
  };

  template <typename T>
  concept allocator_type = std::derived_from<T, Allocator>;

} // namespace soul::memory
