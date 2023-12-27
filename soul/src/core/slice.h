#pragma once

#include "core/type.h"
#include "core/vector.h"

namespace soul
{

  template <typename T>
  class Slice
  {
  public:
    Slice() = default;

    Slice(Vector<T>* array, usize begin, usize end)
        : vector_(array), begin_idx_(begin), end_idx_(end), size_(end_idx_ - begin_idx_)
    {
    }

    Slice(const Slice& other) = default;
    auto operator=(const Slice& other) -> Slice& = default;
    Slice(Slice&& other) noexcept = default;
    auto operator=(Slice&& other) noexcept -> Slice& = default;
    ~Slice() = default;

    auto set(Vector<T>* array, usize begin, usize end) -> void
    {
      vector_ = array;
      begin_idx_ = begin;
      end_idx_ = end;
      size_ = end_idx_ - begin_idx_;
    }

    [[nodiscard]]
    auto
    operator[](usize idx) -> T&
    {
      SOUL_ASSERT(0, idx < size_);
      return (*vector_)[begin_idx_ + idx];
    }

    [[nodiscard]]
    auto
    operator[](usize idx) const -> const T&
    {
      SOUL_ASSERT(0, idx < size_);
      return this->operator[](idx);
    }

    [[nodiscard]]
    auto size() const -> usize
    {
      return size_;
    }

    [[nodiscard]]
    auto begin() const -> const T*
    {
      return vector_->data() + begin_idx_;
    }
    [[nodiscard]]
    auto end() const -> const T*
    {
      return vector_->data() + end_idx_;
    }

    [[nodiscard]]
    auto begin() -> T*
    {
      return vector_->data() + begin_idx_;
    }
    [[nodiscard]]
    auto end() -> T*
    {
      return vector_->data() + end_idx_;
    }

    [[nodiscard]]
    auto get_begin_idx() const -> usize
    {
      return begin_idx_;
    }
    [[nodiscard]]
    auto get_end_idx() const -> usize
    {
      return end_idx_;
    }

  private:
    Vector<T>* vector_ = nullptr;
    usize begin_idx_ = 0;
    usize end_idx_ = 0;
    usize size_ = 0;
  };

} // namespace soul
