#include <algorithm>
#include <cstdarg>
#include <cstdio>

#include "core/config.h"
#include "core/cstring.h"

namespace soul
{

  CString::CString(memory::Allocator* allocator) : allocator_(allocator) {}

  CString::CString(soul_size size, memory::Allocator& allocator)
      : allocator_(&allocator), size_(size)
  {
    reserve(size + 1);
    data_[size_] = '\0';
  }

  CString::CString(char* str, memory::Allocator& allocator) : CString(strlen(str), allocator)
  {
    memcpy(data_, str, capacity_ * sizeof(char));
  }

  CString::CString(const char* str, memory::Allocator& allocator) : CString(strlen(str), allocator)
  {
    memcpy(data_, str, capacity_ * sizeof(char));
  }

  CString::CString(char* str) : CString(str, *get_default_allocator()) {}

  CString::CString(const char* str) : CString(str, *get_default_allocator()) {}

  CString::CString(const CString& rhs)
  {
    allocator_ = rhs.allocator_;
    data_ = static_cast<char*>(allocator_->allocate(rhs.capacity_, alignof(char)));
    size_ = rhs.size_;
    capacity_ = rhs.capacity_;
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

  auto CString::operator=(char* buf) -> CString&
  {
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
    const auto new_data = static_cast<char*>(allocator_->allocate(new_capacity, alignof(char)));
    if (data_ != nullptr) {
      memcpy(new_data, data_, capacity_ * sizeof(char));
      allocator_->deallocate(data_);
    }
    data_ = new_data;
    capacity_ = new_capacity;
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

  auto CString::appendf(SOUL_FORMAT_STRING const char* format, ...) -> CString&
  {
    va_list args;

    va_start(args, format);
    const auto needed = vsnprintf(nullptr, 0, format, args);
    va_end(args);

    if (needed + 1 > capacity_ - size_) {
      const auto new_capacity =
        capacity_ >= (needed + 1) ? 2 * capacity_ : capacity_ + (needed + 1);
      reserve(new_capacity);
    }

    va_start(args, format);
    vsnprintf(data_ + size_, (needed + 1), format, args);
    va_end(args);

    size_ += needed;
    data_[size_] = '\0';

    return *this;
  }

} // namespace soul
