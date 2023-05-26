#pragma once

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
    using this_type = CString;
    using value_type = char;
    using pointer = char*;
    using const_pointer = const char*;
    using reference = char&;
    using const_reference = const char&;

    explicit CString(soul_size size, memory::Allocator& allocator = *get_default_allocator());
    CString(const char*, memory::Allocator& allocator);

    explicit CString(memory::Allocator* allocator = get_default_allocator());
    CString(const char*); // NOLINT(hicpp-explicit-conversions)

    CString(const CString&);
    auto operator=(const CString&) -> CString&;
    CString(CString&&) noexcept;
    auto operator=(CString&&) noexcept -> CString&;
    auto operator=(const char*) -> CString&;
    ~CString();
    auto swap(CString&) noexcept -> void;
    friend auto swap(CString& a, CString& b) noexcept -> void { a.swap(b); }

    auto reserve(soul_size new_capacity) -> void;

    auto push_back(char c) -> void;
    auto append(const CString& x) -> CString&;
    auto append(const char* x) -> CString&;

    [[nodiscard]] auto capacity() const -> soul_size { return capacity_; }
    [[nodiscard]] auto size() const -> soul_size { return size_; }

    [[nodiscard]] auto data() -> pointer { return data_; }
    [[nodiscard]] auto data() const -> const_pointer { return data_; }

  private:
    memory::Allocator* allocator_;
    soul_size capacity_ = 0;
    soul_size size_ = 0; // string size, not counting NULL
    char* data_ = nullptr;
  };
} // namespace soul
