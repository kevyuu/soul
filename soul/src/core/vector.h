#pragma once

#include <array>
#include <memory>

#include "core/config.h"
#include "core/dev_util.h"
#include "core/util.h"
#include "memory/allocator.h"

namespace soul
{

  template <typename T, memory::allocator_type AllocatorType = memory::Allocator, soul_size N = 0>
  class Vector
  {
  public:
    using this_type = Vector<T, AllocatorType, N>;
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr soul_size INLINE_ELEMENT_COUNT = N;

    explicit Vector(AllocatorType* allocator = get_default_allocator());
    explicit Vector(soul_size size, AllocatorType& allocator = *get_default_allocator());
    Vector(
      soul_size size, const value_type& val, AllocatorType& allocator = *get_default_allocator());
    Vector(const Vector& other);
    Vector(const Vector& other, AllocatorType& allocator);
    Vector(Vector&& other) noexcept;

    template <std::input_iterator Iterator>
    Vector(Iterator first, Iterator last, AllocatorType& allocator = *get_default_allocator());

    template <soul_size ArraySize>
    Vector(std::array<T, ArraySize>&& arr, AllocatorType& allocator = *get_default_allocator());

    auto operator=(const Vector& rhs) -> Vector&;
    auto operator=(Vector&& other) noexcept -> Vector&;
    ~Vector();

    auto assign(soul_size size, const value_type& value) -> void;

    template <std::input_iterator InputIterator>
    auto assign(InputIterator first, InputIterator last) -> void;

    auto swap(this_type& other) noexcept -> void;
    friend auto swap(this_type& a, this_type& b) noexcept -> void { a.swap(b); }

    [[nodiscard]] auto begin() -> iterator { return buffer_; }
    [[nodiscard]] auto begin() const -> const_iterator { return buffer_; }
    [[nodiscard]] auto cbegin() const -> const_iterator { return buffer_; }

    [[nodiscard]] auto end() -> iterator { return buffer_ + size_; }
    [[nodiscard]] auto end() const -> const_iterator { return buffer_ + size_; }
    [[nodiscard]] auto cend() const -> const_iterator { return buffer_ + size_; }

    [[nodiscard]] auto rbegin() -> reverse_iterator { return reverse_iterator(end()); }
    [[nodiscard]] auto rbegin() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cend());
    }
    [[nodiscard]] auto crbegin() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cend());
    }

    [[nodiscard]] auto rend() -> reverse_iterator { return reverse_iterator(begin()); }
    [[nodiscard]] auto rend() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cbegin());
    }
    [[nodiscard]] auto crend() const -> const_reverse_iterator
    {
      return const_reverse_iterator(cbegin());
    }

    [[nodiscard]] auto capacity() const noexcept -> soul_size { return capacity_; }
    [[nodiscard]] auto size() const noexcept -> soul_size { return size_; }
    [[nodiscard]] auto empty() const noexcept -> bool { return size_ == 0; }

    auto set_allocator(AllocatorType& allocator) noexcept -> void;
    [[nodiscard]] auto get_allocator() const noexcept -> AllocatorType*;

    auto resize(soul_size size) -> void;
    auto reserve(soul_size capacity) -> void;

    auto clear() noexcept -> void;
    auto cleanup() -> void;

    auto push_back(const T& item) -> void;
    auto push_back() -> reference;
    auto push_back(T&& item) -> void;
    auto pop_back() -> void;
    auto pop_back(soul_size count) -> void;

    auto add(const T& item) -> soul_size;
    auto add(T&& item) -> soul_size;

    template <typename... Args>
    auto emplace_back(Args&&... args) -> reference;

    auto append(const this_type& other) -> void;

    template <std::input_iterator InputIterator>
    auto append(InputIterator first, InputIterator last) -> void;

    [[nodiscard]] auto data() noexcept -> pointer { return buffer_; }
    [[nodiscard]] auto data() const noexcept -> const_pointer { return buffer_; }

    [[nodiscard]] auto front() -> reference
    {
      SOUL_ASSERT(0, size_ != 0, "Vector cannot be empty when calling Vector::front()");
      return buffer_[0];
    }

    [[nodiscard]] auto front() const -> const_reference
    {
      SOUL_ASSERT(0, size_ != 0, "Vector cannot be empty when calling Vector::front()");
      return buffer_[0];
    }

    [[nodiscard]] auto back() noexcept -> reference
    {
      SOUL_ASSERT(0, size_ != 0, "Vector cannot be empty when calling Vector::back()");
      return buffer_[size_ - 1];
    }

    [[nodiscard]] auto back() const noexcept -> const_reference
    {
      SOUL_ASSERT(0, size_ != 0, "Vector cannot be empty when calling Vector::back()");
      return buffer_[size_ - 1];
    }

    [[nodiscard]] auto operator[](soul_size idx) -> reference
    {
      SOUL_ASSERT(
        0,
        idx < size_,
        "Out of bound access to array detected. idx = %llu, _size = %llu",
        idx,
        size_);
      return buffer_[idx];
    }

    [[nodiscard]] auto operator[](soul_size idx) const -> const_reference
    {
      SOUL_ASSERT(
        0,
        idx < size_,
        "Out of bound access to array detected. idx = %llu, _size=%llu",
        idx,
        size_);
      return buffer_[idx];
    }

  private:
    static auto get_new_capacity(soul_size old_capacity) -> soul_size;
    auto init_reserve(soul_size capacity) -> void;
    [[nodiscard]] auto is_using_stack_storage() const -> bool;

    SOUL_NO_UNIQUE_ADDRESS RawBuffer<T, N> stack_storage_;

    static constexpr soul_size GROWTH_FACTOR = 2;
    static constexpr bool IS_SBO = N > 0;
    AllocatorType* allocator_ = nullptr;
    T* buffer_ = stack_storage_.data();
    soul_size size_ = 0;
    soul_size capacity_ = N;
  };

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  Vector<T, AllocatorType, N>::Vector(AllocatorType* allocator) : allocator_(allocator)
  {
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  Vector<T, AllocatorType, N>::Vector(soul_size size, AllocatorType& allocator)
      : allocator_(&allocator)
  {
    init_reserve(size);
    std::uninitialized_value_construct_n(buffer_, size);
    size_ = size;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  Vector<T, AllocatorType, N>::Vector(
    soul_size size, const value_type& val, AllocatorType& allocator)
      : allocator_(&allocator)
  {
    init_reserve(size);
    std::uninitialized_fill_n(buffer_, size, val);
    size_ = size;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  Vector<T, AllocatorType, N>::Vector(const Vector& other)
      : allocator_(other.allocator_), size_(other.size_)
  {
    init_reserve(other.capacity_);
    std::uninitialized_copy_n(other.buffer_, other.size_, buffer_);
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  Vector<T, AllocatorType, N>::Vector(const Vector& other, AllocatorType& allocator)
      : allocator_(&allocator), size_(other.size_)
  {
    init_reserve(other.capacity_);
    std::uninitialized_copy_n(other.buffer_, other.size_, buffer_);
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  Vector<T, AllocatorType, N>::Vector(Vector&& other) noexcept
      : allocator_(std::exchange(other.allocator_, nullptr))
  {
    if (!other.is_using_stack_storage()) {
      buffer_ = std::exchange(other.buffer_, other.stack_storage_.data());
      capacity_ = std::exchange(other.capacity_, N);
    } else {
      std::uninitialized_move_n(other.buffer_, other.size_, buffer_);
    }
    size_ = std::exchange(other.size_, 0);
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  template <std::input_iterator Iterator>
  Vector<T, AllocatorType, N>::Vector(Iterator first, Iterator last, AllocatorType& allocator)
      : allocator_(&allocator)
  {
    if constexpr (std::forward_iterator<Iterator>) {
      const soul_size size = std::distance(first, last);
      init_reserve(size);
      std::uninitialized_copy(first, last, buffer_);
      size_ = size;
    } else {
      for (; first != last; ++first) {
        push_back(*first);
      }
    }
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  template <soul_size ArraySize>
  Vector<T, AllocatorType, N>::Vector(std::array<T, ArraySize>&& arr, AllocatorType& allocator)
      : allocator_(&allocator)
  {
    init_reserve(ArraySize);
    std::uninitialized_copy(
      std::make_move_iterator(std::begin(arr)), std::make_move_iterator(std::end(arr)), buffer_);
    size_ = ArraySize;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::operator=(const Vector& rhs) -> this_type&
  {
    // NOLINT(bugprone-unhandled-self-assignment) use copy and swap idiom
    this_type(rhs, *allocator_).swap(*this);
    return *this;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::operator=(Vector&& other) noexcept -> this_type&
  {
    if (this->allocator_ == other.allocator_) {
      this_type(std::move(other)).swap(*this);
    } else {
      this_type(other, *allocator_).swap(*this);
    }
    return *this;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  Vector<T, AllocatorType, N>::~Vector()
  {
    cleanup();
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::assign(soul_size size, const value_type& value) -> void
  {
    if (size > capacity_) {
      this_type tmp(size, value, *allocator_);
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

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  template <std::input_iterator InputIterator>
  auto Vector<T, AllocatorType, N>::assign(InputIterator first, InputIterator last) -> void
  {
    const soul_size new_size = std::distance(first, last);
    if constexpr (std::random_access_iterator<InputIterator>) {
      if (new_size > capacity_) {
        T* new_buffer = allocator_->template allocate_array<T>(new_size);
        std::uninitialized_copy(first, last, new_buffer);
        std::destroy_n(buffer_, size_);
        allocator_->deallocate_array(buffer_, capacity_);
        buffer_ = new_buffer;
        size_ = new_size;
        capacity_ = new_size;
      } else if (new_size > size_) {
        std::copy_n(first, size_, buffer_);
        std::uninitialized_copy(first + size_, first + new_size, buffer_ + size_);
        size_ = new_size;
      } else {
        std::copy_n(first, new_size, buffer_);
        std::destroy(buffer_ + new_size, buffer_ + size_);
        size_ = new_size;
      }
    } else {
      iterator buffer_start(buffer_);
      iterator buffer_end(buffer_ + size_);
      while ((buffer_start != buffer_end) && (first != last)) {
        *buffer_start = *first;
        ++first;
        ++buffer_start;
      }
      if (first == last) {
        pop_back(std::distance(buffer_start, buffer_end));
      } else {
        append(first, last);
      }
    }
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::swap(this_type& other) noexcept -> void
  {
    using std::swap;

    if (!is_using_stack_storage() && !other.is_using_stack_storage()) {
      SOUL_ASSERT(
        0, allocator_ == other.allocator_, "Cannot swap container with different allocator");
      swap(buffer_, other.buffer_);
    } else if (is_using_stack_storage() && !other.is_using_stack_storage()) {
      std::uninitialized_move_n(buffer_, size_, other.stack_storage_.data());
      buffer_ = other.buffer_;
      other.buffer_ = other.stack_storage_.data();
    } else if (!is_using_stack_storage() && other.is_using_stack_storage()) {
      std::uninitialized_move_n(other.buffer_, other.size_, stack_storage_.data());
      other.buffer_ = buffer_;
      buffer_ = stack_storage_.data();
    } else {
      RawBuffer<T, N> temp;
      std::uninitialized_move_n(other.buffer_, other.size_, temp.data());
      std::uninitialized_move_n(buffer_, size_, other.buffer_);
      std::uninitialized_move_n(temp.data(), other.size_, buffer_);
    }

    swap(size_, other.size_);
    swap(capacity_, other.capacity_);
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::set_allocator(AllocatorType& allocator) noexcept -> void
  {
    if (buffer_ != stack_storage_.data()) {
      T* buffer = allocator.template allocate_array<T>(size_);
      std::uninitialized_move_n(buffer_, size_, buffer);
      allocator_->deallocate_array(buffer_, capacity_);
      buffer_ = buffer;
    }
    allocator_ = &allocator;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::get_allocator() const noexcept -> AllocatorType*
  {
    return allocator_;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::resize(soul_size size) -> void
  {
    if (size > size_) {
      if (size > capacity_) {
        reserve(size);
      }
      std::uninitialized_value_construct(buffer_ + size_, buffer_ + size);
    } else {
      std::destroy(buffer_ + size, buffer_ + size_);
    }
    size_ = size;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::reserve(soul_size capacity) -> void
  {
    if (capacity < capacity_) {
      return;
    }
    if (!IS_SBO || capacity > N) {
      T* old_buffer = buffer_;
      buffer_ = allocator_->template allocate_array<T>(capacity);
      std::uninitialized_move_n(old_buffer, size_, buffer_);
      if (old_buffer != stack_storage_.data()) {
        allocator_->deallocate_array(old_buffer, capacity_);
      }
      capacity_ = capacity;
    }
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::clear() noexcept -> void
  {
    std::destroy_n(buffer_, size_);
    size_ = 0;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::cleanup() -> void
  {
    clear();
    if (buffer_ != stack_storage_.data()) {
      allocator_->deallocate_array(buffer_, capacity_);
    }
    buffer_ = stack_storage_.data();
    capacity_ = N;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::push_back(const T& item) -> void
  {
    T val(item);
    push_back(std::move(val));
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::push_back() -> reference
  {
    if (size_ == capacity_) {
      reserve(get_new_capacity(capacity_));
    }
    pointer item = buffer_ + size_;
    new (item) T();
    ++size_;
    return *item;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::push_back(T&& item) -> void
  {
    if (size_ == capacity_) {
      reserve(get_new_capacity(capacity_));
    }
    new (buffer_ + size_) T(std::move(item));
    ++size_;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::pop_back() -> void
  {
    SOUL_ASSERT(0, size_ != 0, "Cannot pop_back an empty sbo_vector");
    size_--;
    std::destroy_n(buffer_ + size_, 1);
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::pop_back(soul_size count) -> void
  {
    SOUL_ASSERT(0, size_ >= count, "Cannot pop back more than sbo_vector size");
    size_ = size_ - count;
    std::destroy_n(buffer_ + size_, count);
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::add(const T& item) -> soul_size
  {
    push_back(item);
    return size_ - 1;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::add(T&& item) -> soul_size
  {
    push_back(std::move(item));
    return size_ - 1;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  template <typename... Args>
  auto Vector<T, AllocatorType, N>::emplace_back(Args&&... args) -> reference
  {
    if (size_ == capacity_) {
      reserve(get_new_capacity(capacity_));
    }
    T* item = (buffer_ + size_);
    new (item) T(std::forward<Args>(args)...);
    ++size_;
    return *item;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::append(const this_type& other) -> void
  {
    append(other.begin(), other.end());
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  template <std::input_iterator InputIterator>
  auto Vector<T, AllocatorType, N>::append(InputIterator first, InputIterator last) -> void
  {
    if constexpr (std::forward_iterator<InputIterator>) {
      const soul_size new_size = std::distance(first, last) + size_;
      reserve(new_size);
      std::uninitialized_copy(first, last, buffer_ + size_);
      size_ = new_size;
    } else {
      for (; first != last; ++first) {
        push_back(*first);
      }
    }
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::get_new_capacity(soul_size old_capacity) -> soul_size
  {
    return old_capacity * GROWTH_FACTOR + 8;
  }

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::init_reserve(soul_size capacity) -> void
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

  template <typename T, memory::allocator_type AllocatorType, soul_size N>
  auto Vector<T, AllocatorType, N>::is_using_stack_storage() const -> bool
  {
    if constexpr (!IS_SBO) {
      return false;
    }
    return buffer_ == stack_storage_.data();
  }

} // namespace soul
