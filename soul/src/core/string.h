#pragma once

#include <format>

#include "core/comp_str.h"
#include "core/compiler.h"
#include "core/config.h"
#include "core/not_null.h"
#include "core/span.h"
#include "core/type.h"

#include "memory/allocator.h"

namespace soul::memory
{
  class Allocator;
}

namespace soul
{

  /** Returns true if this C string pointer is definitely located in the constant program data
  segment and does not require memory management. Used by SIMDString. */
  inline auto is_in_const_segment(NotNull<const char*> str) -> b8
  {
    static const char* test_str = "__A Unique ConstSeg String__";
    static const auto PROBED_CONST_SEG_ADDR = uintptr_t(test_str);
    // MSVC assigns the const_seg to very high addresses that are grouped together.
    // Beware that when using explicit SEGMENT
    // https://docs.microsoft.com/en-us/cpp/assembler/masm/segment?view=vs-2017 or const_seg
    // https://docs.microsoft.com/en-us/cpp/preprocessor/const-seg?view=vs-2017, this does not work.
    // However, that simply disables some optimizations in G3DString, it doesn't break it.
    //
    // Assume it is at least 5 MB long.
    return (std::labs(static_cast<long>(uintptr_t(str.get()) - PROBED_CONST_SEG_ADDR)) < 5000000L);
  }

  inline constexpr auto str_length(NotNull<const char*> str) -> usize
  {
    if (std::is_constant_evaluated()) {
      usize length = 0;
      while (str.get()[length] != '\0') {
        length++;
      }
      return length;
    } else {
      return strlen(str.get());
    }
  }

  namespace impl
  {
    template <typename CharT, typename... Args>
    inline constexpr auto estimate_args_length(Args&&... args) -> usize
    {
      constexpr auto estimate_arg_length = []<typename Arg>(const Arg arg) -> usize {
        if constexpr (is_same_v<Arg, std::basic_string_view<CharT>>) {
          return arg.size();
        } else if constexpr (is_same_v<Arg, const CharT*>) {
          return 32;
        } else {
          return 8;
        }
      };
      return (estimate_arg_length(args) + ...);
    }

  } // namespace impl

  template <memory::allocator_type AllocatorT = memory::Allocator, usize InlineCapacityV = 8>
  class BasicString
  {
  public:
    using value_type = char;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;

    static_assert(InlineCapacityV > 0);
    static constexpr usize INLINE_CAPACITY = InlineCapacityV;

    explicit constexpr BasicString(NotNull<memory::Allocator*> allocator = get_default_allocator())
        : allocator_(allocator), capacity_(get_init_capacity(1))
    {
      init_reserve(capacity_);
      data()[0] = '\0';
    }

    constexpr BasicString( // NOLINT(hicpp-explicit-conversions)
      CompStr str,
      NotNull<memory::Allocator*> allocator = get_default_allocator())
        : allocator_(allocator), size_(str.size()), capacity_(0)
    {
      storage_.data = const_cast<pointer>(str.data()); // NOLINT
    }

    constexpr BasicString(BasicString&& other) noexcept { swap(other); }

    constexpr auto operator=(BasicString&& other) noexcept -> BasicString&
    {
      swap(other);
      other.clear();
      return *this;
    }

    constexpr auto operator=(CompStr str) noexcept -> BasicString&
    {
      size_ = str.size();
      maybe_deallocate();
      storage_.data =
        const_cast<pointer>(str.data()); // NOLINT(cppcoreguidelines-pro-type-const-cast)
      capacity_ = 0;
      return *this;
    }

    constexpr void assign(NotNull<const char*> str) { assign(StringView{str.get(), strlen(str)}); }

    constexpr void assign(StringView str_view)
    {
      const char* str = str_view.data();
      size_ = str_view.size();

      if (is_in_const_segment(str)) {
        maybe_deallocate();
        storage_.data = const_cast<pointer>(str); // NOLINT
        capacity_ = 0;
      } else {
        maybe_reallocate(size_ + 1);
        memcpy(data(), str, size_ + 1);
      }
    }

    template <typename... Args>
    constexpr void assignf(std::format_string<Args...> fmt, Args&&... args)
    {
      clear();
      std::vformat_to(std::back_inserter(*this), fmt.get(), std::make_format_args(args...));
    }

    constexpr ~BasicString()
    {
      if (is_using_heap()) {
        allocator_->deallocate_array(storage_.data, capacity_);
      }
    }

    [[nodiscard]]
    static constexpr auto WithCapacity(
      usize capacity, NotNull<AllocatorT*> allocator = get_default_allocator()) -> BasicString
    {
      return BasicString(Construct::with_capacity, capacity, allocator);
    }

    template <typename... Args>
    [[nodiscard]]
    static constexpr auto Format(std::format_string<Args...> fmt, Args&&... args) -> BasicString
    {
      return BasicString(Construct::vformat, fmt.get(), std::make_format_args(args...));
    }

    template <typename... Args>
    [[nodiscard]]
    static constexpr auto ReservedFormat(
      NotNull<AllocatorT*> allocator, std::format_string<Args...> fmt, Args&&... args)
      -> BasicString
    {
      return BasicString(
        Construct::reserved_format, allocator, std::move(fmt), std::forward<Args>(args)...);
    }

    [[nodiscard]]
    static constexpr auto From(
      NotNull<const char*> str, NotNull<AllocatorT*> allocator = get_default_allocator())
      -> BasicString
    {
      return BasicString(Construct::from, StringView{str, strlen(str)}, allocator);
    }

    [[nodiscard]]
    static constexpr auto From(
      StringView str_view, NotNull<AllocatorT*> allocator = get_default_allocator()) -> BasicString
    {
      return BasicString(Construct::from, str_view, allocator);
    }

    [[nodiscard]]
    static constexpr auto UnsharedFrom(
      NotNull<const char*> str, NotNull<AllocatorT*> allocator = get_default_allocator())
      -> BasicString
    {
      return BasicString(Construct::unshared_from, StringView{str, strlen(str)}, allocator);
    }

    [[nodiscard]]
    static constexpr auto UnsharedFrom(
      StringView str_view, NotNull<AllocatorT*> allocator = get_default_allocator()) -> BasicString
    {
      return BasicString(Construct::unshared_from, str_view, allocator);
    }

    [[nodiscard]]
    static constexpr auto WithSize(
      usize size, NotNull<AllocatorT*> allocator = get_default_allocator()) -> BasicString
    {
      return BasicString(Construct::with_size, size, allocator);
    }

    [[nodiscard]]
    constexpr auto clone() const -> BasicString
    {
      return BasicString(*this);
    }

    constexpr auto clone_from(const BasicString& other) { *this = other; }

    constexpr void swap(BasicString& other) noexcept
    {
      using std::swap;
      swap(size_, other.size_);
      swap(capacity_, other.capacity_);
      swap(allocator_, other.allocator_);
      swap(storage_, other.storage_);
    }

    friend void swap(BasicString& a, BasicString& b) noexcept { a.swap(b); }

    constexpr void reserve(usize new_capacity)
    {
      if (new_capacity > capacity_) {
        if (new_capacity > InlineCapacityV) {
          const b8 was_using_heap = is_using_heap();
          const auto old_data = data();
          usize old_capacity = capacity_;

          capacity_ = new_capacity;
          const auto new_data = is_using_stack_storage()
                                  ? storage_.buffer
                                  : allocator_->template allocate_array<value_type>(capacity_);
          std::memcpy(new_data, old_data, size_ + 1);
          if (is_using_heap()) {
            storage_.data = new_data;
          }
          if (was_using_heap) {
            allocator_->deallocate_array(storage_.data, old_capacity);
          }
        } else if (is_using_const_segment()) {
          std::memcpy(storage_.buffer, data(), size_ + 1);
          capacity_ = InlineCapacityV;
        } else {
          unreachable();
        }
      }
    }

    constexpr void clear()
    {
      if (is_using_const_segment()) {
        capacity_ = INLINE_CAPACITY;
      }
      size_ = 0;
      data()[size_] = '\0';
    }

    constexpr void push_back(char c)
    {
      ensure_capacity(size_ + 2);
      data()[size_] = c;
      size_ += 1;
      data()[size_] = '\0';
    }

    constexpr auto append(const BasicString& other) -> BasicString&
    {
      ensure_capacity(size_ + other.size_ + 1);
      memcpy(data() + size_, other.data(), other.size_ + 1);
      size_ += other.size_;
      data()[size_] = '\0';
      return *this;
    }

    constexpr auto append(const char* x) -> BasicString&
    {
      const auto extra_size = soul::str_length(x);
      ensure_capacity(size_ + extra_size + 1);
      std::memcpy(data() + size_, x, extra_size + 1);
      size_ += extra_size;
      return *this;
    }

    template <typename... Args>
    constexpr void appendf(std::format_string<Args...> fmt, Args&&... args)
    {
      std::format_to(std::back_inserter(*this), fmt, std::forward<Args>(args)...);
    }

    [[nodiscard]]
    constexpr auto capacity() const -> usize
    {
      return capacity_;
    }

    [[nodiscard]]
    constexpr auto size() const -> usize
    {
      return size_;
    }

    [[nodiscard]]
    constexpr auto data() -> pointer
    {
      if (is_using_stack_storage()) {
        return storage_.buffer;
      } else {
        return storage_.data;
      }
    }

    [[nodiscard]]
    constexpr auto data() const -> const_pointer
    {
      if (is_using_stack_storage()) {
        return storage_.buffer;
      } else {
        return storage_.data;
      }
    }

    [[nodiscard]]
    constexpr auto c_str() const -> const_pointer
    {
      return data();
    }

    template <ts_unsigned_integral SpanSizeT = usize>
    [[nodiscard]]
    constexpr auto span() -> Span<pointer>
    {
      return {data(), cast<SpanSizeT>(size())};
    }

    template <ts_unsigned_integral SpanSizeT = usize>
    [[nodiscard]]
    constexpr auto span() const -> Span<const_pointer>
    {
      return {data(), cast<SpanSizeT>(size())};
    }

    template <ts_unsigned_integral SpanSizeT = usize>
    [[nodiscard]]
    constexpr auto cspan() const -> Span<const_pointer>
    {
      return {data(), cast<SpanSizeT>(size())};
    }

    constexpr friend auto operator==(const BasicString& lhs, const BasicString& rhs) -> b8
    {
      if (lhs.size_ != rhs.size_) {
        return false;
      }
      const auto lhs_data = lhs.data();
      const auto rhs_data = rhs.data();
      for (usize i = 0; i < lhs.size_; i++) {
        if (lhs_data[i] != rhs_data[i]) {
          return false;
        }
      }
      return true;
    }

  private:
    union {
      value_type buffer[InlineCapacityV];
      pointer data;
    } storage_;

    AllocatorT* allocator_ = nullptr;
    usize size_ = 0; // string size, not counting NULL
    usize capacity_ = InlineCapacityV;

    struct Construct {
      struct WithCapacity {
      };
      static constexpr auto with_capacity = WithCapacity{};

      struct ReservedFormat {
      };
      static constexpr auto reserved_format = ReservedFormat{};

      struct Vformat {
      };
      static constexpr auto vformat = Vformat{};

      struct From {
      };
      static constexpr auto from = From{};

      struct UnsharedFrom {
      };
      static constexpr auto unshared_from = UnsharedFrom{};

      struct WithSize {
      };
      static constexpr auto with_size = WithSize{};
    };

    constexpr BasicString(
      Construct::WithCapacity /*tag*/, usize capacity, NotNull<AllocatorT*> allocator)
        : allocator_(allocator), capacity_(get_init_capacity(capacity))
    {
      init_reserve(capacity_);
      data()[0] = '\0';
    }

    constexpr BasicString(Construct::Vformat /*tag*/, std::string_view fmt, std::format_args args)
        : allocator_(get_default_allocator())
    {
      data()[0] = '\0';
      std::vformat_to(std::back_inserter(*this), fmt, args);
    }

    template <typename... Args>
    constexpr BasicString(
      Construct::ReservedFormat /*tag*/,
      NotNull<AllocatorT*> allocator,
      std::format_string<Args...> fmt,
      Args&&... args)
        : allocator_(allocator),
          size_(std::formatted_size(fmt, args...)),
          capacity_(get_init_capacity(size_ + 1))
    {
      init_reserve(capacity_);
      std::format_to_n(data(), size_, fmt, std::forward<Args>(args)...);
      data()[size_] = '\0';
    }

    constexpr BasicString(
      Construct::From /*tag*/, StringView str_view, NotNull<AllocatorT*> allocator)
        : allocator_(allocator), size_(str_view.size())
    {
      const char* str = str_view.data();
      if (is_in_const_segment(str)) {
        storage_.data = const_cast<pointer>(str); // NOLINT
        capacity_ = 0;
      } else {
        capacity_ = get_init_capacity(size_ + 1);
        init_reserve(capacity_);
        std::memcpy(data(), str, size_ + 1);
      }
    }

    constexpr BasicString(
      Construct::UnsharedFrom /*tag*/, StringView str_view, NotNull<AllocatorT*> allocator)
        : allocator_(allocator), size_(str_view.size()), capacity_(get_init_capacity(size_ + 1))
    {

      init_reserve(capacity_);
      std::memcpy(data(), str_view.data(), size_ + 1);
    }

    constexpr BasicString(Construct::WithSize /*tag*/, usize size, NotNull<AllocatorT*> allocator)
        : size_(size), capacity_(get_init_capacity(size + 1)), allocator_(allocator)
    {
      init_reserve(capacity_);
      data()[size_] = '\0';
    }

    constexpr BasicString(const BasicString& other)
        : size_(other.size_), allocator_(other.allocator_)
    {
      if (other.is_using_const_segment()) {
        storage_.data = other.storage_.data;
        capacity_ = 0;
      } else {
        capacity_ = get_init_capacity(size_ + 1);
        init_reserve(capacity_);
        size_ = other.size_;
        std::memcpy(data(), other.data(), size_ + 1);
      }
    }

    constexpr auto operator=(const BasicString& other) -> BasicString&
    {
      if (&other == this) {
        return *this;
      }
      if (other.is_using_const_segment()) {
        maybe_deallocate();
        storage_.data = other.storage_.data;
        size_ = other.size_;
        capacity_ = other.capacity_;
      } else {
        size_ = other.size_;
        maybe_reallocate(size_ + 1);
        std::memcpy(data(), other.data(), size_ + 1);
      }
      return *this;
    }

    [[nodiscard]]
    SOUL_ALWAYS_INLINE constexpr auto is_using_const_segment() const -> b8
    {
      return capacity_ == 0;
    }

    [[nodiscard]]
    SOUL_ALWAYS_INLINE constexpr auto is_using_heap() const -> b8
    {
      return capacity_ > InlineCapacityV;
    }

    [[nodiscard]]
    SOUL_ALWAYS_INLINE constexpr auto is_using_stack_storage() const -> b8
    {
      return capacity_ == InlineCapacityV;
    }

    [[nodiscard]]
    SOUL_ALWAYS_INLINE static constexpr auto get_new_capacity(usize min_capacity)
    {
      return (min_capacity <= InlineCapacityV)
               ? InlineCapacityV
               : std::max(2 * min_capacity, 2 * InlineCapacityV + 1);
    }

    [[nodiscard]]
    SOUL_ALWAYS_INLINE static constexpr auto get_init_capacity(usize min_capacity)
    {
      return (min_capacity <= InlineCapacityV) ? InlineCapacityV : min_capacity;
    }

    constexpr void init_reserve(usize capacity)
    {
      if (capacity > InlineCapacityV) {
        storage_.data = allocator_->template allocate_array<value_type>(capacity);
      }
    }

    constexpr void prepare_to_mutate()
    {
      if (is_using_const_segment()) {
        const_pointer old = storage_.data;
        capacity_ = get_new_capacity(size_ + 1);
        init_reserve(capacity_);
        std::memcpy(data(), old, size_ + 1);
      }
    }

    constexpr void ensure_capacity(usize min_capacity)
    {
      if (capacity_ < min_capacity) {
        const auto was_using_heap = is_using_heap();
        const auto old_data = data();
        const auto old_capacity = capacity_;
        capacity_ = get_new_capacity(min_capacity);
        pointer new_data = is_using_stack_storage()
                             ? storage_.buffer
                             : allocator_->template allocate_array<value_type>(capacity_);
        std::memcpy(new_data, old_data, size_ + 1);
        if (is_using_heap()) {
          storage_.data = new_data;
        }
        if (was_using_heap) {
          allocator_->deallocate_array(old_data, old_capacity);
        }
      }
    }

    constexpr void create_gap(usize min_capacity, usize count, usize count2, usize pos)
    {
      if (
        ((capacity_ < min_capacity) &&
         !(is_using_stack_storage() && (min_capacity < InlineCapacityV))) ||
        is_using_const_segment()) {

        const auto was_using_heap = is_using_heap();
        const auto old = data();
        const auto old_capacity = capacity_;
        capacity_ = get_new_capacity(min_capacity + 1);
        const auto new_data = is_using_stack_storage()
                                ? storage_.buffer
                                : allocator_->template allocate_array<value_type>(capacity_);
        std::memcpy(new_data, old, pos);
        std::memcpy(new_data + pos + count2, old + pos + count, size_ - pos - count + 1);
        if (is_using_heap()) {
          construct_at(&storage_.data, new_data);
        }
        if (was_using_heap) {
          allocator_->deallocate_array(old, old_capacity);
        }
      } else {
        memmove(data() + pos + count2, data() + pos + count, size_ - pos - count + 1);
      }
    }

    constexpr void maybe_deallocate()
    {
      if (is_using_heap()) {
        allocator_->deallocate_array(storage_.data, capacity_);
        capacity_ = InlineCapacityV;
      }
    }

    constexpr void maybe_reallocate(usize min_capacity)
    {
      if (capacity_ != 0 && capacity_ >= min_capacity) {
        return;
      }

      if (is_using_heap()) {
        allocator_->deallocate_array(storage_.data, capacity_);
      }

      capacity_ = get_new_capacity(min_capacity);
      init_reserve(capacity_);
    }

    friend constexpr void soul_op_hash_combine(auto& hasher, const BasicString& val)
    {
      hasher.combine_span(val.cspan());
    }
  };

  using String = BasicString<memory::Allocator, 64>;

} // namespace soul

template <soul::memory::allocator_type AllocatorT, soul::usize InlineCapacityV>
struct std::formatter<soul::BasicString<AllocatorT, InlineCapacityV>> // NOLINT
    : std::formatter<std::string_view> {
  auto format(
    const soul::BasicString<AllocatorT, InlineCapacityV>& string, std::format_context& ctx)
  {
    return std::formatter<std::string_view>::format(
      std::string_view(string.data(), string.size()), ctx);
  }
};
