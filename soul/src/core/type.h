#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include <glm/glm.hpp>

#include "core/aabb.h"
#include "core/builtins.h"
#include "core/compiler.h"
#include "core/matrix.h"
#include "core/panic_lite.h"
#include "core/quaternion.h"
#include "core/type_traits.h"
#include "core/vec.h"

namespace soul
{

  constexpr usize ONE_KILOBYTE = 1024;
  constexpr usize ONE_MEGABYTE = 1024 * ONE_KILOBYTE;
  constexpr usize ONE_GIGABYTE = 1024 * ONE_MEGABYTE;

  template <typename T>
  concept ts_bit_block = std::unsigned_integral<T>;

  template <typename T, usize N>
  struct RawBuffer {
    alignas(T) std::byte buffer[sizeof(T) * N];

    auto data() -> T* { return reinterpret_cast<T*>(buffer); }

    auto data() const -> const T* { return reinterpret_cast<const T*>(buffer); }
  };

  template <typename T>
  struct RawBuffer<T, 0> {
    auto data() -> T* { return nullptr; }

    auto data() const -> const T* { return nullptr; }
  };

  template <typename PointerDst, typename PointerSrc>
    requires std::is_pointer_v<PointerDst> && std::is_pointer_v<PointerSrc>
  constexpr auto cast(PointerSrc srcPtr) -> PointerDst
  {
    using Dst = std::remove_pointer_t<PointerDst>;
    if constexpr (!std::is_same_v<PointerDst, void*>) {
      SOUL_ASSERT_LITE(
        0,
        reinterpret_cast<uptr>(srcPtr) % alignof(Dst) == 0,
        "Source pointer is not aligned in PointerDst alignment!");
    }
    // ReSharper disable once CppCStyleCast
    return (PointerDst)(srcPtr);
  }

  template <std::integral IntegralDst, std::integral IntegralSrc>
  constexpr auto cast(IntegralSrc src) -> IntegralDst
  {
    SOUL_ASSERT_LITE(
      0,
      static_cast<u64>(src) <= std::numeric_limits<IntegralDst>::max(),
      "Source value is larger than the destintation type maximum!");
    SOUL_ASSERT_LITE(
      0,
      static_cast<i64>(src) >= std::numeric_limits<IntegralDst>::min(),
      "Source value is smaller than the destination type minimum!");
    return static_cast<IntegralDst>(src);
  }

  template <typename PointerDst, typename PointerSrc>
    requires std::is_pointer_v<PointerDst> && std::is_pointer_v<PointerSrc>
  constexpr auto downcast(PointerSrc srcPtr) -> PointerDst
  {
    return static_cast<PointerDst>(srcPtr);
  }

  template <ts_scoped_enum E>
  constexpr auto to_underlying(E e) noexcept
  {
    return static_cast<std::underlying_type_t<E>>(e);
  }

  template <
    typename ResourceType,
    typename IDType,
    IDType NullValue = std::numeric_limits<IDType>::max()>
  struct ID {
    using store_type = IDType;
    IDType id;

    constexpr ID() : id(NullValue) {}

    constexpr explicit ID(IDType id) : id(id) {}

    template <std::integral Integral>
    constexpr explicit ID(Integral id) : id(soul::cast<IDType>(id))
    {
    }

    template <typename Pointer>
      requires std::is_pointer_v<Pointer>
    constexpr explicit ID(Pointer id) : id(id)
    {
    }

    friend auto operator<=>(const ID& lhs, const ID& rhs) = default;

    [[nodiscard]]
    auto is_null() const -> bool
    {
      return id == NullValue;
    }
    [[nodiscard]]
    auto is_valid() const -> bool
    {
      return id != NullValue;
    }

    static constexpr auto null() -> ID { return ID(NullValue); }

    friend void soul_op_hash_combine(auto& hasher, const ID& val) { hasher.combine(val.id); }
  };

  template <ts_flag Flag>
  class FlagIter
  {
    using store_type = std::underlying_type_t<Flag>;

  public:
    class Iterator
    {
    public:
      constexpr explicit Iterator(store_type index) : index_(index) {}

      constexpr auto operator++() -> Iterator
      {
        ++index_;
        return *this;
      }

      constexpr auto operator==(const Iterator& other) const -> bool = default;

      constexpr auto operator*() const -> Flag { return Flag(index_); }

    private:
      store_type index_;
    };

    [[nodiscard]]
    auto begin() const -> Iterator
    {
      return Iterator(0);
    }
    [[nodiscard]]
    auto end() const -> Iterator
    {
      return Iterator(to_underlying(Flag::COUNT));
    }

    constexpr FlagIter() = default;

    static auto Iterates() -> FlagIter { return FlagIter(); }

    static auto Count() -> u64 { return to_underlying(Flag::COUNT); }
  };

} // namespace soul
