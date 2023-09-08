#pragma once

#include <ranges>

#include "core/config.h"
#include "core/objops.h"
#include "core/own_ref.h"
#include "core/panic.h"
#include "core/span.h"
#include "core/util.h"
#include "memory/allocator.h"

namespace soul
{

  template <
    typename T,
    memory::allocator_type AllocatorT = memory::Allocator,
    usize inline_element_count = 0>
  class Vector
  {
  public:
    using this_type = Vector<T, AllocatorT, inline_element_count>;
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr usize INLINE_ELEMENT_COUNT = inline_element_count;

    explicit Vector(AllocatorT* allocator = get_default_allocator());
    Vector(Vector&& other) noexcept;
    auto operator=(Vector&& other) noexcept -> Vector&;
    ~Vector();

    [[nodiscard]]
    static auto WithSize(usize size, AllocatorT& allocator = *get_default_allocator()) -> this_type;

    [[nodiscard]]
    static auto WithCapacity(usize capacity, AllocatorT& allocator = *get_default_allocator())
      -> this_type;

    template <std::ranges::input_range RangeT>
    [[nodiscard]]
    static auto From(RangeT&& range, NotNull<AllocatorT*> allocator = get_default_allocator())
      -> this_type;

    [[nodiscard]]
    static auto FillN(
      usize size, const value_type& val, AllocatorT& allocator = *get_default_allocator())
      -> this_type
      requires ts_copy<T>;

    template <ts_generate_fn<T> Fn>
    [[nodiscard]]
    static auto GenerateN(Fn fn, usize size, AllocatorT& allocator = *get_default_allocator())
      -> this_type;

    template <std::ranges::input_range RangeT, typename Fn>
    [[nodiscard]]
    static auto Transform(RangeT&& range, Fn fn, AllocatorT& allocator = *get_default_allocator())
      -> this_type;

    template <typename Fn>
    [[nodiscard]]
    static auto TransformIndex(
      usize idx_start,
      usize idx_end,
      Fn fn,
      NotNull<AllocatorT*> allocator = get_default_allocator()) -> this_type;

    [[nodiscard]]
    auto clone() const -> this_type;

    [[nodiscard]]
    auto clone(memory::Allocator& allocator) const -> this_type;

    void clone_from(const this_type& other);

    void assign(usize size, const value_type& value)
      requires ts_copy<T>;

    template <std::ranges::input_range RangeT>
    void assign(RangeT&& range);

    void swap(this_type& other) noexcept;

    friend void swap(this_type& a, this_type& b) noexcept { a.swap(b); }

    [[nodiscard]]
    auto begin() -> iterator;

    [[nodiscard]]
    auto begin() const -> const_iterator;

    [[nodiscard]]
    auto cbegin() const -> const_iterator;

    [[nodiscard]]
    auto end() -> iterator;

    [[nodiscard]]
    auto end() const -> const_iterator;

    [[nodiscard]]
    auto cend() const -> const_iterator;

    [[nodiscard]]
    auto rbegin() -> reverse_iterator;

    [[nodiscard]]
    auto rbegin() const -> const_reverse_iterator;

    [[nodiscard]]
    auto crbegin() const -> const_reverse_iterator;

    [[nodiscard]]
    auto rend() -> reverse_iterator;

    [[nodiscard]]
    auto rend() const -> const_reverse_iterator;

    [[nodiscard]]
    auto crend() const -> const_reverse_iterator;

    [[nodiscard]]
    auto capacity() const noexcept -> usize;

    [[nodiscard]]
    auto size() const noexcept -> usize;

    [[nodiscard]]
    auto size_in_bytes() const noexcept -> usize;

    [[nodiscard]]
    auto empty() const noexcept -> b8;

    void set_allocator(AllocatorT& allocator) noexcept;

    [[nodiscard]]
    auto get_allocator() const noexcept -> AllocatorT*;

    void resize(usize size);

    void reserve(usize capacity);

    void push_back(OwnRef<T> item);

    void generate_back(ts_generate_fn<T> auto fn);

    void pop_back();

    void pop_back(usize count);

    void shrink_to_fit();

    void clear() noexcept;

    void cleanup();

    [[nodiscard]]
    auto add(OwnRef<T> item) -> usize;

    template <typename... Args>
    auto emplace_back(Args&&... args) -> reference;

    template <std::ranges::input_range RangeT>
    void append(RangeT&& range);

    [[nodiscard]]
    auto data() noexcept -> pointer;

    [[nodiscard]]
    auto data() const noexcept -> const_pointer;

    template <ts_unsigned_integral SpanSizeT>
    [[nodiscard]]
    auto span() -> Span<pointer, SpanSizeT>;

    template <ts_unsigned_integral SpanSizeT>
    [[nodiscard]]
    auto span() const -> Span<const_pointer, SpanSizeT>;

    template <ts_unsigned_integral SpanSizeT>
    [[nodiscard]]
    auto cspan() const -> Span<const_pointer, SpanSizeT>;

    [[nodiscard]]
    auto front() -> reference;

    [[nodiscard]]
    auto front() const -> const_reference;

    [[nodiscard]]
    auto back() noexcept -> reference;

    [[nodiscard]]
    auto back() const noexcept -> const_reference;

    [[nodiscard]]
    auto
    operator[](usize idx) -> reference;

    [[nodiscard]]
    auto
    operator[](usize idx) const -> const_reference;

  private:
    SOUL_NO_UNIQUE_ADDRESS RawBuffer<T, inline_element_count> stack_storage_;
    static constexpr usize GROWTH_FACTOR = 2;
    static constexpr b8 IS_SBO = inline_element_count > 0;
    AllocatorT* allocator_ = nullptr;
    T* buffer_ = stack_storage_.data();
    usize size_ = 0;
    usize capacity_ = inline_element_count;

    Vector(const Vector& other, AllocatorT& allocator);

    Vector(const Vector& other);

    auto operator=(const Vector& rhs) -> Vector&;

    struct Construct {
      struct WithSize {
      };
      struct WithCapacity {
      };
      struct From {
      };
      struct FillN {
      };
      struct GenerateN {
      };
      struct Transform {
      };
      struct TransformIndex {
      };

      static constexpr auto with_size = WithSize{};
      static constexpr auto with_capacity = WithCapacity{};
      static constexpr auto from = From{};
      static constexpr auto fill_n = FillN{};
      static constexpr auto generate_n = GenerateN{};
      static constexpr auto transform = Transform{};
      static constexpr auto transform_index = TransformIndex{};
    };

    explicit Vector(
      Construct::WithSize tag, usize size, AllocatorT& allocator = *get_default_allocator());

    explicit Vector(
      Construct::FillN tag,
      const value_type& val,
      usize size,
      AllocatorT& allocator = *get_default_allocator());

    template <ts_generate_fn<T> Fn>
    explicit Vector(
      Construct::GenerateN tag,
      Fn fn,
      usize size,
      AllocatorT& allocator = *get_default_allocator());

    explicit Vector(
      Construct::WithCapacity tag,
      usize capacity,
      AllocatorT& allocator = *get_default_allocator());

    template <std::ranges::input_range RangeT>
    explicit Vector(
      Construct::From tag,
      RangeT&& range,
      NotNull<AllocatorT*> allocator = get_default_allocator());

    template <std::ranges::input_range RangeT, typename Fn>
    explicit Vector(
      Construct::Transform tag,
      RangeT&& range,
      Fn fn,
      AllocatorT& allocator = *get_default_allocator());

    template <typename Fn>
    explicit Vector(
      Construct::TransformIndex tag,
      usize idx_start,
      usize idx_end,
      Fn fn,
      NotNull<AllocatorT*> allocator = get_default_allocator());

    [[nodiscard]]
    static auto get_new_capacity(usize old_capacity) -> usize;

    [[nodiscard]]
    static auto get_new_capacity(usize old_capacity, usize minimum_capacity) -> usize;

    void init_reserve(usize capacity);

    [[nodiscard]]
    auto is_using_stack_storage() const -> b8;
  };

  template <typename T, memory::allocator_type AllocatorT, usize N>
  Vector<T, AllocatorT, N>::Vector(AllocatorT* allocator) : allocator_(allocator)
  {
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  Vector<T, AllocatorT, N>::Vector(const Vector& other)
      : allocator_(other.allocator_), size_(other.size_)
  {
    init_reserve(other.capacity_);
    if constexpr (ts_clone<T>) {
      uninitialized_clone_n(other.buffer_, other.size_, buffer_);
    } else {
      uninitialized_copy_n(other.buffer_, other.size_, buffer_);
    }
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  Vector<T, AllocatorT, N>::Vector(const Vector& other, AllocatorT& allocator)
      : allocator_(&allocator), size_(other.size_)
  {
    init_reserve(other.capacity_);
    if constexpr (ts_clone<T>) {
      uninitialized_clone_n(other.buffer_, other.size_, buffer_);
    } else {
      uninitialized_copy_n(other.buffer_, other.size_, buffer_);
    }
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  Vector<T, AllocatorT, N>::Vector(Vector&& other) noexcept
      : allocator_(std::exchange(other.allocator_, nullptr))
  {
    if (!other.is_using_stack_storage()) {
      buffer_ = std::exchange(other.buffer_, other.stack_storage_.data());
      capacity_ = std::exchange(other.capacity_, N);
    } else {
      uninitialized_move_n(other.buffer_, other.size_, buffer_);
    }
    size_ = std::exchange(other.size_, 0);
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  Vector<T, AllocatorT, N>::Vector(Construct::WithSize /*tag*/, usize size, AllocatorT& allocator)
      : allocator_(&allocator), size_(size)
  {
    init_reserve(size);
    uninitialized_value_construct_n(buffer_, size);
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  Vector<T, AllocatorT, N>::Vector(
    Construct::WithCapacity /*tag*/, usize capacity, AllocatorT& allocator)
      : allocator_(&allocator)
  {
    init_reserve(capacity);
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  Vector<T, AllocatorT, N>::Vector(
    Construct::FillN /*tag*/, const value_type& val, usize size, AllocatorT& allocator)
      : allocator_(&allocator), size_(size)
  {
    init_reserve(size);
    std::uninitialized_fill_n(buffer_, size, val);
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  template <ts_generate_fn<T> Fn>
  Vector<T, AllocatorT, N>::Vector(
    Construct::GenerateN /*tag*/, Fn fn, usize size, AllocatorT& allocator)
      : allocator_(&allocator), size_(size)
  {
    init_reserve(size);
    uninitialized_generate_n(fn, size, buffer_);
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  template <std::ranges::input_range RangeT>
  Vector<T, AllocatorT, N>::Vector(
    Construct::From /*tag*/, RangeT&& range, NotNull<AllocatorT*> allocator)
      : allocator_(allocator)
  {
    if constexpr (std::ranges::sized_range<RangeT> || std::ranges::forward_range<RangeT>) {
      const auto size = usize(std::ranges::distance(range));
      init_reserve(size);
      uninitialized_copy_n(std::ranges::begin(range), size, buffer_);
      size_ = size;
    } else {
      for (auto it = std::ranges::begin(range), last = std::ranges::end(range); it != last; it++) {
        epmplace_back(*it);
      }
    }
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  template <std::ranges::input_range RangeT, typename Fn>
  Vector<T, AllocatorT, N>::Vector(
    Construct::Transform /*tag*/, RangeT&& range, Fn fn, AllocatorT& allocator)
      : allocator_(&allocator)
  {
    if constexpr (std::ranges::sized_range<RangeT> || std::ranges::forward_range<RangeT>) {
      const auto size = usize(std::ranges::distance(range));
      init_reserve(size);
      uninitialized_transform_construct_n(std::ranges::begin(range), std::move(fn), size, buffer_);
      size_ = size;
    } else {
      for (auto it = std::ranges::begin(range), last = std::ranges::end(range); it != last; it++) {
        transform_construct_at(buffer_, it, std::move(fn));
      }
    }
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  template <typename Fn>
  Vector<T, AllocatorT, N>::Vector(
    Construct::TransformIndex /*tag*/,
    usize idx_start,
    usize idx_end,
    Fn fn,
    NotNull<AllocatorT*> allocator)
      : allocator_(allocator), size_(idx_end - idx_start)
  {
    SOUL_ASSERT(idx_end >= idx_start, "Index end must be larget than index start");
    init_reserve(size_);
    uninitialized_transform_index_construct(idx_start, idx_end, std::move(fn), buffer_);
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  auto Vector<T, AllocatorT, N>::operator=(const Vector& rhs) -> this_type&
  {
    // NOLINT(bugprone-unhandled-self-assignment) use copy and swap idiom
    this_type(rhs, *allocator_).swap(*this);
    return *this;
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  auto Vector<T, AllocatorT, N>::operator=(Vector&& other) noexcept -> this_type&
  {
    if (this->allocator_ == other.allocator_) {
      this_type(std::move(other)).swap(*this);
    } else {
      this_type(other, *allocator_).swap(*this);
    }
    return *this;
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  Vector<T, AllocatorT, N>::~Vector()
  {
    cleanup();
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  auto Vector<T, AllocatorT, N>::WithSize(usize size, AllocatorT& allocator) -> this_type
  {
    return this_type(Construct::with_size, size, allocator);
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  auto Vector<T, AllocatorT, N>::FillN(usize size, const value_type& val, AllocatorT& allocator)
    -> this_type
    requires ts_copy<T>
  {
    return this_type(Construct::fill_n, val, size, allocator);
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  template <ts_generate_fn<T> Fn>
  auto Vector<T, AllocatorT, N>::GenerateN(Fn fn, usize size, AllocatorT& allocator) -> this_type
  {
    return this_type(Construct::generate_n, fn, size, allocator);
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::clone() const -> this_type
  {
    return Vector(*this);
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::clone(memory::Allocator& allocator) const
    -> this_type
  {
    return Vector(*this, allocator);
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  auto Vector<T, AllocatorT, N>::WithCapacity(usize capacity, AllocatorT& allocator) -> this_type
  {
    return this_type(Construct::with_capacity, capacity, allocator);
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  template <std::ranges::input_range RangeT>
  [[nodiscard]]
  auto Vector<T, AllocatorT, N>::From(RangeT&& range, NotNull<AllocatorT*> allocator) -> this_type
  {
    return this_type(Construct::from, std::forward<RangeT>(range), allocator);
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  template <std::ranges::input_range RangeT, typename Fn>
  [[nodiscard]]
  auto Vector<T, AllocatorT, N>::Transform(RangeT&& range, Fn fn, AllocatorT& allocator)
    -> this_type
  {
    return this_type(Construct::transform, std::forward<RangeT>(range), std::move(fn), allocator);
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  template <typename Fn>
  [[nodiscard]]
  auto Vector<T, AllocatorT, N>::TransformIndex(
    usize idx_start, usize idx_end, Fn fn, NotNull<AllocatorT*> allocator) -> this_type
  {
    return this_type(Construct::transform_index, idx_start, idx_end, std::move(fn), allocator);
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  void Vector<T, AllocatorT, inline_element_count>::clone_from(const this_type& other)
  {
    *this = other;
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  void Vector<T, AllocatorT, N>::assign(usize size, const value_type& value)
    requires ts_copy<T>
  {
    if (size > capacity_) {
      this_type tmp(Construct::fill_n, value, size, *allocator_);
      swap(tmp);
    } else if (size > size_) {
      std::fill_n(buffer_, size_, value);
      std::uninitialized_fill_n(buffer_ + size_, size - size_, value);
      size_ = size;
    } else {
      std::fill_n(buffer_, size, value);
      std::destroy(buffer_ + size, buffer_ + size_);
      size_ = size;
    }
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  template <std::ranges::input_range RangeT>
  void Vector<T, AllocatorT, N>::assign(RangeT&& range)
  {
    if constexpr (std::ranges::sized_range<RangeT> || std::ranges::forward_range<RangeT>) {

      const auto new_size = soul::cast<usize>(std::ranges::distance(range));

      if (new_size > capacity_) {

        T* new_buffer = allocator_->template allocate_array<T>(new_size);
        uninitialized_copy_n(std::begin(range), new_size, new_buffer);
        std::destroy_n(buffer_, size_);
        if (!is_using_stack_storage()) {
          allocator_->deallocate_array(buffer_, capacity_);
        }
        buffer_ = new_buffer;
        size_ = new_size;
        capacity_ = new_size;

      } else if (new_size > size_) {
        auto range_input_it = std::ranges::begin(range);

        if constexpr (std::ranges::random_access_range<RangeT>) {
          std::copy_n(range_input_it, size_, buffer_);
          range_input_it += size_;
        } else {
          for (auto i = 0; i < size_; ++i, ++range_input_it) {
            buffer_[i] = *range_input_it;
          }
        }

        uninitialized_copy_n(range_input_it, new_size - size_, buffer_ + size_);
        size_ = new_size;

      } else {
        std::copy_n(std::ranges::begin(range), new_size, buffer_);
        std::destroy(buffer_ + new_size, buffer_ + size_);
        size_ = new_size;
      }
    } else {

      iterator buffer_start(buffer_);
      iterator buffer_end(buffer_ + size_);
      auto first = std::ranges::begin(range);
      const auto last = std::ranges::end(range);
      while ((buffer_start != buffer_end) && (first != last)) {
        *buffer_start = *first;
        ++first;
        ++buffer_start;
      }

      if (first == last) {
        pop_back(std::distance(buffer_start, buffer_end));
      } else {
        append(std::ranges::subrange(first, last));
      }
    }
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  void Vector<T, AllocatorT, N>::swap(this_type& other) noexcept
  {
    using std::swap;

    if (!is_using_stack_storage() && !other.is_using_stack_storage()) {
      SOUL_ASSERT(
        0, allocator_ == other.allocator_, "Cannot swap container with different allocator");
      swap(buffer_, other.buffer_);
    } else if (is_using_stack_storage() && !other.is_using_stack_storage()) {
      uninitialized_move_n(buffer_, size_, other.stack_storage_.data());
      buffer_ = other.buffer_;
      other.buffer_ = other.stack_storage_.data();
    } else if (!is_using_stack_storage() && other.is_using_stack_storage()) {
      uninitialized_move_n(other.buffer_, other.size_, stack_storage_.data());
      other.buffer_ = buffer_;
      buffer_ = stack_storage_.data();
    } else {
      RawBuffer<T, N> temp;
      uninitialized_move_n(other.buffer_, other.size_, temp.data());
      uninitialized_move_n(buffer_, size_, other.buffer_);
      uninitialized_move_n(temp.data(), other.size_, buffer_);
    }

    swap(size_, other.size_);
    swap(capacity_, other.capacity_);
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::begin() -> iterator
  {
    return buffer_;
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::begin() const -> const_iterator
  {
    return buffer_;
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::cbegin() const -> const_iterator
  {
    return buffer_;
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::end() -> iterator
  {
    return buffer_ + size_;
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::end() const -> const_iterator
  {
    return buffer_ + size_;
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::cend() const -> const_iterator
  {
    return buffer_ + size_;
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::rbegin() -> reverse_iterator
  {
    return reverse_iterator(end());
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::rbegin() const -> const_reverse_iterator
  {
    return const_reverse_iterator(cend());
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::crbegin() const -> const_reverse_iterator
  {
    return const_reverse_iterator(cend());
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::rend() -> reverse_iterator
  {
    return reverse_iterator(begin());
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::rend() const -> const_reverse_iterator
  {
    return const_reverse_iterator(cbegin());
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::crend() const -> const_reverse_iterator
  {
    return const_reverse_iterator(cbegin());
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::capacity() const noexcept -> usize
  {
    return capacity_;
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::size() const noexcept -> usize
  {
    return size_;
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::size_in_bytes() const noexcept -> usize
  {
    return size_ * sizeof(T);
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::empty() const noexcept -> b8
  {
    return size_ == 0;
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  void Vector<T, AllocatorT, N>::set_allocator(AllocatorT& allocator) noexcept
  {
    if (buffer_ != stack_storage_.data()) {
      T* buffer = allocator.template allocate_array<T>(size_);
      uninitialized_move_n(buffer_, size_, buffer);
      allocator_->deallocate_array(buffer_, capacity_);
      buffer_ = buffer;
    }
    allocator_ = &allocator;
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  auto Vector<T, AllocatorT, N>::get_allocator() const noexcept -> AllocatorT*
  {
    return allocator_;
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  void Vector<T, AllocatorT, N>::resize(usize size)
  {
    if (size > size_) {
      if (size > capacity_) {
        reserve(size);
      }
      uninitialized_value_construct_n(buffer_ + size_, size - size_);
    } else {
      std::destroy(buffer_ + size, buffer_ + size_);
    }
    size_ = size;
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  void Vector<T, AllocatorT, N>::reserve(usize capacity)
  {
    if (capacity < capacity_) {
      return;
    }
    if (!IS_SBO || capacity > N) {
      T* old_buffer = buffer_;
      buffer_ = allocator_->template allocate_array<T>(capacity);
      uninitialized_move_n(old_buffer, size_, buffer_);
      if (old_buffer != stack_storage_.data()) {
        allocator_->deallocate_array(old_buffer, capacity_);
      }
      capacity_ = capacity;
    }
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  void Vector<T, AllocatorT, N>::clear() noexcept
  {
    std::destroy_n(buffer_, size_);
    size_ = 0;
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  void Vector<T, AllocatorT, N>::cleanup()
  {
    clear();
    if (buffer_ != stack_storage_.data()) {
      allocator_->deallocate_array(buffer_, capacity_);
    }
    buffer_ = stack_storage_.data();
    capacity_ = N;
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  void Vector<T, AllocatorT, N>::push_back(OwnRef<T> item)
  {
    if (size_ == capacity_) {
      const auto new_capacity = get_new_capacity(capacity_);
      T* old_buffer = buffer_;
      buffer_ = allocator_->template allocate_array<T>(new_capacity);
      item.store_at(buffer_ + size_);
      uninitialized_move_n(old_buffer, size_, buffer_);
      if (old_buffer != stack_storage_.data()) {
        allocator_->deallocate_array(old_buffer, capacity_);
      }
      capacity_ = new_capacity;
    } else {
      item.store_at(buffer_ + size_);
    }
    ++size_;
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  void Vector<T, AllocatorT, N>::generate_back(ts_generate_fn<T> auto fn)
  {
    if (size_ == capacity_) {
      const auto new_capacity = get_new_capacity(capacity_);
      T* old_buffer = buffer_;
      buffer_ = allocator_->template allocate_array<T>(new_capacity);
      generate_at(buffer_ + size_, fn);
      uninitialized_move_n(old_buffer, size_, buffer_);
      if (old_buffer != stack_storage_.data()) {
        allocator_->deallocate_array(old_buffer, capacity_);
      }
      capacity_ = new_capacity;
    } else {
      generate_at(buffer_ + size_, fn);
    }
    ++size_;
  }
  template <typename T, memory::allocator_type AllocatorT, usize N>
  void Vector<T, AllocatorT, N>::pop_back()
  {
    SOUL_ASSERT(0, size_ != 0, "Cannot pop_back an empty sbo_vector");
    size_--;
    std::destroy_n(buffer_ + size_, 1);
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  void Vector<T, AllocatorT, N>::pop_back(usize count)
  {
    SOUL_ASSERT(0, size_ >= count, "Cannot pop back more than sbo_vector size");
    size_ = size_ - count;
    std::destroy_n(buffer_ + size_, count);
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  void Vector<T, AllocatorT, inline_element_count>::shrink_to_fit()
  {
    if (capacity_ == size_ || is_using_stack_storage()) {
      return;
    }
    T* old_buffer = buffer_;
    const auto old_capacity = capacity_;
    if (size_ > inline_element_count) {
      buffer_ = allocator_->template allocate_array<T>(size_);
      capacity_ = size_;
    } else {
      buffer_ = stack_storage_.data();
      capacity_ = inline_element_count;
    }
    uninitialized_move_n(old_buffer, size_, buffer_);
    allocator_->deallocate_array(old_buffer, old_capacity);
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  auto Vector<T, AllocatorT, N>::add(OwnRef<T> item) -> usize
  {
    push_back(std::move(item));
    return size_ - 1;
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  template <typename... Args>
  auto Vector<T, AllocatorT, N>::emplace_back(Args&&... args) -> reference
  {
    if (size_ == capacity_) {
      const auto new_capacity = get_new_capacity(capacity_);
      T* old_buffer = buffer_;
      buffer_ = allocator_->template allocate_array<T>(new_capacity);
      soul::construct_at(buffer_ + size_, std::forward<Args>(args)...);
      uninitialized_move_n(old_buffer, size_, buffer_);
      if (old_buffer != stack_storage_.data()) {
        allocator_->deallocate_array(old_buffer, capacity_);
      }
      capacity_ = new_capacity;
    } else {
      soul::construct_at(buffer_ + size_, std::forward<Args>(args)...);
    }
    ++size_;
    return *(buffer_ + size_ - 1);
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  template <std::ranges::input_range RangeT>
  void Vector<T, AllocatorT, N>::append(RangeT&& range)
  {
    if constexpr (std::ranges::sized_range<RangeT> || std::ranges::forward_range<RangeT>) {
      const auto range_size = usize(std::ranges::distance(range));
      const auto new_size = size_ + range_size;
      if (new_size > capacity_) {
        const auto new_capacity = get_new_capacity(capacity_, new_size);
        T* old_buffer = buffer_;
        buffer_ = allocator_->template allocate_array<T>(new_capacity);
        uninitialized_copy_n(std::ranges::begin(range), range_size, buffer_ + size_);
        uninitialized_move_n(old_buffer, size_, buffer_);
        if (old_buffer != stack_storage_.data()) {
          allocator_->deallocate_array(old_buffer, capacity_);
        }
        capacity_ = new_capacity;
      } else {
        uninitialized_copy_n(std::ranges::begin(range), range_size, buffer_ + size_);
      }
      size_ = new_size;
    } else {
      for (auto it = std::ranges::begin(range), last = std::ranges::end(range); it != last; it++) {
        emplace_back(*it);
      }
    }
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::data() noexcept -> pointer
  {
    return buffer_;
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::data() const noexcept -> const_pointer
  {
    return buffer_;
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  template <ts_unsigned_integral SpanSizeT>
  auto Vector<T, AllocatorT, inline_element_count>::span() -> Span<pointer, SpanSizeT>
  {
    return {data(), cast<SpanSizeT>(size())};
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  template <ts_unsigned_integral SpanSizeT>
  auto Vector<T, AllocatorT, inline_element_count>::span() const -> Span<const_pointer, SpanSizeT>
  {
    return {data(), cast<SpanSizeT>(size())};
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  template <ts_unsigned_integral SpanSizeT>
  auto Vector<T, AllocatorT, inline_element_count>::cspan() const -> Span<const_pointer, SpanSizeT>
  {
    return {data(), cast<SpanSizeT>(size())};
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::front() -> reference
  {
    SOUL_ASSERT(0, size_ != 0, "Vector cannot be empty when calling Vector::front()");
    return buffer_[0];
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::front() const -> const_reference
  {
    SOUL_ASSERT(0, size_ != 0, "Vector cannot be empty when calling Vector::front()");
    return buffer_[0];
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::back() noexcept -> reference
  {
    SOUL_ASSERT(0, size_ != 0, "Vector cannot be empty when calling Vector::back()");
    return buffer_[size_ - 1];
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::back() const noexcept -> const_reference
  {
    SOUL_ASSERT(0, size_ != 0, "Vector cannot be empty when calling Vector::back()");
    return buffer_[size_ - 1];
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::operator[](usize idx) -> reference
  {
    SOUL_ASSERT(
      0,
      idx < size_,
      "Out of bound access to array detected. idx = %llu, _size = %llu",
      idx,
      size_);
    return buffer_[idx];
  }

  template <typename T, memory::allocator_type AllocatorT, usize inline_element_count>
  auto Vector<T, AllocatorT, inline_element_count>::operator[](usize idx) const -> const_reference
  {
    SOUL_ASSERT(
      0, idx < size_, "Out of bound access to array detected. idx = %llu, _size=%llu", idx, size_);
    return buffer_[idx];
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  auto Vector<T, AllocatorT, N>::get_new_capacity(usize old_capacity) -> usize
  {
    return old_capacity * GROWTH_FACTOR + 8;
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  auto Vector<T, AllocatorT, N>::get_new_capacity(usize old_capacity, usize minimum_capacity)
    -> usize
  {
    const auto new_capacity = old_capacity * GROWTH_FACTOR + 8;
    return (new_capacity > minimum_capacity) ? new_capacity : minimum_capacity;
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  void Vector<T, AllocatorT, N>::init_reserve(usize capacity)
  {
    auto need_heap = true;
    if constexpr (IS_SBO) {
      need_heap = capacity > N;
    }
    if (need_heap) {
      buffer_ = allocator_->template allocate_array<T>(capacity);
      capacity_ = capacity;
    }
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  auto Vector<T, AllocatorT, N>::is_using_stack_storage() const -> b8
  {
    if constexpr (!IS_SBO) {
      return false;
    }
    return buffer_ == stack_storage_.data();
  }

  template <typename T, memory::allocator_type AllocatorT, usize N>
  auto operator==(const Vector<T, AllocatorT, N>& lhs, const Vector<T, AllocatorT, N>& rhs) -> b8
  {
    if (lhs.size() != rhs.size()) {
      return false;
    }
    return std::ranges::equal(lhs, rhs);
  }
} // namespace soul
