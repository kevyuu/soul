#pragma once

#include <format>

#include "core/config.h"
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

    explicit CString(usize size, memory::Allocator& allocator = *get_default_allocator());

    CString(const char*, memory::Allocator& allocator);

    explicit CString(memory::Allocator* allocator = get_default_allocator());

    CString(const char*); // NOLINT(hicpp-explicit-conversions)

    CString(CString&&) noexcept;

    auto operator=(CString&&) noexcept -> CString&;

    auto operator=(const char*) -> CString&;

    ~CString();

    [[nodiscard]]
    auto clone() const -> CString;

    auto clone_from(const CString& other);

    auto swap(CString&) noexcept -> void;

    friend auto swap(CString& a, CString& b) noexcept -> void { a.swap(b); }

    auto reserve(usize new_capacity) -> void;

    auto push_back(char c) -> void;

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
    usize capacity_ = 0;
    usize size_ = 0; // string size, not counting NULL
    char* data_ = nullptr;

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
  void CString::appendf(std::format_string<Args...> fmt, Args&&... args)
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
