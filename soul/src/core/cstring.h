#pragma once

#include <format>

#include "core/config.h"
#include "core/not_null.h"
#include "core/type.h"

namespace soul::memory
{
  class Allocator;
}

namespace soul
{
  class CString
  {
  public:
    using value_type = char;
    using pointer = char*;
    using const_pointer = const char*;
    using reference = char&;
    using const_reference = const char&;

    explicit CString(NotNull<memory::Allocator*> allocator = get_default_allocator());

    CString(CString&&) noexcept;

    auto operator=(CString&&) noexcept -> CString&;

    void assign(const char*);

    ~CString();

    [[nodiscard]]
    static auto with_capacity(
      usize capacity, NotNull<memory::Allocator*> allocator = get_default_allocator()) -> CString;

    template <typename... Args>
    [[nodiscard]]
    static auto with_capacity_then_format(
      usize capacity,
      NotNull<memory::Allocator*> allocator,
      std::format_string<Args...> fmt,
      Args&&... args) -> CString;

    [[nodiscard]]
    static auto from(
      NotNull<const char*> str, NotNull<memory::Allocator*> allocator = get_default_allocator())
      -> CString;

    [[nodiscard]]
    static auto with_size(
      usize size, NotNull<memory::Allocator*> allocator = get_default_allocator()) -> CString;

    [[nodiscard]]
    auto clone() const -> CString;

    auto clone_from(const CString& other);

    void swap(CString&) noexcept;

    friend void swap(CString& a, CString& b) noexcept { a.swap(b); }

    void reserve(usize new_capacity);

    void push_back(char c);

    auto append(const CString& x) -> CString&;

    auto append(const char* x) -> CString&;

    template <typename... Args>
    void appendf(std::format_string<Args...> fmt, Args&&... args);

    [[nodiscard]]
    auto capacity() const -> usize;

    [[nodiscard]]
    auto size() const -> usize;

    [[nodiscard]]
    auto data() -> pointer;

    [[nodiscard]]
    auto data() const -> const_pointer;

  private:
    struct HeapLayout {
      value_type* data;
      usize capacity;
      usize size;
    };

    struct SSOLayout {
      value_type data[sizeof(HeapLayout) - sizeof(char)];
      char sso_flag;
    };

    memory::Allocator* allocator_;
    usize size_ = 0; // string size, not counting NULL
    usize capacity_ = 0;
    char* data_ = nullptr;

    struct Construct {
      struct WithCapacity {
      };
      static constexpr auto with_capacity = WithCapacity{};

      struct WithCapacityThenFormat {
      };
      static constexpr auto with_capacity_then_format = WithCapacityThenFormat{};

      struct From {
      };
      static constexpr auto from = From{};

      struct WithSize {
      };
      static constexpr auto with_size = WithSize{};
    };

    CString(Construct::WithCapacity tag, usize capacity, NotNull<memory::Allocator*> allocator);

    template <typename... Args>
    CString(
      Construct::WithCapacityThenFormat /* tag */,
      usize capacity,
      memory::Allocator* allocator,
      std::format_string<Args...> fmt,
      Args&&... args);

    CString(Construct::From tag, NotNull<const char*> str, NotNull<memory::Allocator*> allocator);

    CString(Construct::WithSize tag, usize size, NotNull<memory::Allocator*> allocator);

    CString(const CString&);

    auto operator=(const CString&) -> CString&;
  };

  [[nodiscard]]
  inline auto CString::clone() const -> CString
  {
    return CString(*this);
  }

  inline auto CString::clone_from(const CString& other) { *this = other; }

  template <typename... Args>
  auto CString::with_capacity_then_format(
    usize capacity,
    NotNull<memory::Allocator*> allocator,
    std::format_string<Args...> fmt,
    Args&&... args) -> CString
  {
    return CString(
      Construct::with_capacity_then_format, capacity, allocator, fmt, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void CString::appendf(std::format_string<Args...> fmt, Args&&... args)
  {
    std::format_to(std::back_inserter(*this), std::move(fmt), std::forward<Args>(args)...);
  }

  template <typename... Args>
  CString::CString(
    Construct::WithCapacityThenFormat,
    usize capacity,
    memory::Allocator* allocator,
    std::format_string<Args...> fmt,
    Args&&... args)
      : CString(Construct::with_capacity, capacity, allocator)
  {
    std::format_to(std::back_inserter(*this), std::move(fmt), std::forward<Args>(args)...);
  }

  [[nodiscard]]
  inline auto CString::capacity() const -> usize
  {
    return capacity_;
  }

  [[nodiscard]]
  inline auto CString::size() const -> usize
  {
    return size_;
  }

  [[nodiscard]]
  inline auto CString::data() -> pointer
  {
    return data_;
  }

  [[nodiscard]]
  inline auto CString::data() const -> const_pointer
  {
    return data_;
  }
} // namespace soul
