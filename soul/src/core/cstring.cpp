#include <algorithm>
#include <cstdarg>
#include <cstdio>

#include "core/cstring.h"
#include "core/dev_util.h"
#include "memory/allocator.h"

namespace soul
{

  CString::CString(memory::Allocator* allocator) : allocator_(allocator)
  {
    reserve(1);
    data_[size_] = '\0';
  }

  CString::CString(soul_size size, memory::Allocator& allocator)
      : allocator_(&allocator), size_(size)
  {
    reserve(size + 1);
    data_[size_] = '\0';
  }

  CString::CString(const char* str, memory::Allocator& allocator) : allocator_(&allocator)
  {
    SOUL_ASSERT(0, str != nullptr, "Constructing CString with nullptr is undefined behaviour");
    size_ = strlen(str);
    reserve(size_ + 1);
    memcpy(data_, str, capacity_ * sizeof(char));
  }

  CString::CString(const char* str) : CString(str, *get_default_allocator()) {}

  CString::CString(const CString& rhs)
      : allocator_(rhs.allocator_),
        capacity_(rhs.capacity_),
        size_(rhs.size_),
        data_(
          rhs.data_ == nullptr
            ? nullptr
            : static_cast<char*>(allocator_->allocate(rhs.capacity_, alignof(char))))
  {
    std::copy(rhs.data_, rhs.data_ + rhs.capacity_, data_);
  }

  auto CString::operator=(const CString& other) -> CString&
  {
    CString(other).swap(*this);
    return *this;
  }

  CString::CString(CString&& rhs) noexcept
  {
    allocator_ = std::exchange(rhs.allocator_, nullptr);
    data_ = std::exchange(rhs.data_, nullptr);
    capacity_ = std::exchange(rhs.capacity_, 0);
    size_ = std::exchange(rhs.size_, 0);
  }

  auto CString::operator=(CString&& other) noexcept -> CString&
  {
    CString(std::move(other)).swap(*this);
    return *this;
  }

  auto CString::operator=(const char* buf) -> CString&
  {
    SOUL_ASSERT(0, buf != nullptr, "Assigning nullptr to CString is undefined behaviour");
    size_ = strlen(buf);
    if (capacity_ < size_ + 1) {
      reserve(size_ + 1);
    }
    std::copy(buf, buf + size_ + 1, data_);
    return *this;
  }

  CString::~CString()
  {
    if (data_ != nullptr) {
      SOUL_ASSERT(0, capacity_ != 0, "");
      allocator_->deallocate(data_);
    }
  }

  auto CString::swap(CString& rhs) noexcept -> void
  {
    using std::swap;
    swap(allocator_, rhs.allocator_);
    swap(capacity_, rhs.capacity_);
    swap(size_, rhs.size_);
    swap(data_, rhs.data_);
  }

  auto CString::reserve(soul_size new_capacity) -> void
  {
    if (capacity_ >= new_capacity) {
      return;
    }
    const auto new_data = static_cast<char*>(allocator_->allocate(new_capacity, alignof(char)));
    if (data_ != nullptr) {
      memcpy(new_data, data_, capacity_ * sizeof(char));
      allocator_->deallocate(data_);
    }
    data_ = new_data;
    capacity_ = new_capacity;
  }

  auto CString::push_back(char c) -> void
  {
    const auto needed = size_ + 1;
    if (needed + 1 > capacity_) {
      const auto new_capacity =
        capacity_ == 0 ? needed + 1 : (((needed / capacity_) + 1) * capacity_);
      reserve(new_capacity);
    }
    data_[size_] = c;
    size_ += 1;
    data_[size_] = '\0';
  }

  auto CString::append(const CString& x) -> CString&
  {
    const auto needed = size_ + x.size_;
    if (needed + 1 > capacity_) {
      const auto new_capacity =
        capacity_ == 0 ? needed + 1 : (((needed / capacity_) + 1) * capacity_);
      reserve(new_capacity);
    }

    memcpy(data_ + size_, x.data_, x.size_);
    size_ += x.size_;
    data_[size_] = '\0';

    return *this;
  }

  auto CString::append(const char* x) -> CString&
  {
    const auto extra_size = strlen(x);
    const auto needed = size_ + extra_size;
    if (needed + 1 > capacity_) {
      const auto new_capacity =
        capacity_ == 0 ? needed + 1 : (((needed / capacity_) + 1) * capacity_);
      reserve(new_capacity);
    }

    memcpy(data_ + size_, x, extra_size);
    size_ += extra_size;
    data_[size_] = '\0';

    return *this;
  }
} // namespace soul
