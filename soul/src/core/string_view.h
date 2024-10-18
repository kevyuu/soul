#pragma once

#include <format>

#include "core/comp_str.h"
#include "core/span.h"

namespace soul
{
  using StringView = Span<const char*>;

  template <>
  class Span<const char*, usize> : public impl::SpanBase<const char*, usize>
  {
  public:
    using value_type             = remove_pointer_t<const char*>;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using iterator               = value_type*;
    using const_iterator         = const value_type*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type              = usize;

    constexpr explicit Span(const char* data)
        : impl::SpanBase<const char*, usize>(
            data == nullptr ? "" : data, data == nullptr ? 0 : strlen(data))
    {
    }

    constexpr Span(CompStr comp_str)
        : impl::SpanBase<const char*, usize>(comp_str.data(), comp_str.size())
    {
    }

    constexpr Span(MaybeNull<pointer> data, size_type size)
        : impl::SpanBase<const char*, usize>(data, size)
    {
      SOUL_ASSERT(
        0, (size != 0 && data != nullptr) || (size == 0), "Non zero size cannot hold nullptr");
    }

    constexpr Span(NotNull<pointer> data, size_type size)
        : impl::SpanBase<const char*, usize>(data, size)
    {
    }

    constexpr Span(pointer data, size_type size) : impl::SpanBase<const char*, usize>(data, size) {}

    constexpr auto is_null_terminated() const -> b8
    {
      return data_[size_] == '\0';
    }

    constexpr Span(NilSpan /* nil */) : Span(nullptr, 0) {} // NOLINT

    auto operator==(CompStr other) const -> b8
    {
      return *this == StringView(other);
    }

    friend constexpr void soul_op_hash_combine(auto& hasher, Span span)
    {
      hasher.combine_span(span);
    }
  };

} // namespace soul

template <>
struct std::formatter<soul::StringView> : std::formatter<std::string_view>
{
  auto format(soul::StringView string_view, std::format_context& ctx) const
  {
    return std::formatter<std::string_view>::format(
      std::string_view(string_view.data(), string_view.size()), ctx);
  }
};
