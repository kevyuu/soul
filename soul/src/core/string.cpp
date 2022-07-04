#include <algorithm>
#include <cstdarg>
#include <cstdio>

#include "core/string.h"
#include "core/config.h"

namespace soul {

	String::String(memory::Allocator* allocator, uint64 initialCapacity) :
		allocator_(allocator), capacity_(0),
		size_(0), data_(nullptr)
	{
		reserve(initialCapacity);
	}

	String::String(memory::Allocator* allocator, char* str) : String(allocator, strlen(str) + 1) {
		size_ = capacity_ - 1;
		memcpy(data_, str, capacity_ * sizeof(char));
	}

	String::String(memory::Allocator* allocator, const char* str) : String(allocator, strlen(str) + 1) {
		size_ = capacity_ - 1;
		memcpy(data_, str, capacity_ * sizeof(char));
	}

	String::String() : String(get_default_allocator(), uint64(0)) {}
	String::String(char* str) : String(get_default_allocator(), str) {}
	String::String(const char* str) : String(get_default_allocator(), str) {}

	String::String(const String& rhs) {
		allocator_ = rhs.allocator_;
		data_ = (char*) allocator_->allocate(rhs.capacity_, alignof(char));
		size_ = rhs.size_;
		capacity_ = rhs.capacity_;
		std::copy(rhs.data_, rhs.data_ + rhs.capacity_, data_);
	}

	String& String::operator=(const String& other) {
		String(other).swap(*this);
		return *this;
	}

	String::String(String&& rhs) noexcept {
		allocator_ = std::exchange(rhs.allocator_, nullptr);
		data_ = std::exchange(rhs.data_, nullptr);
		capacity_ = std::exchange(rhs.capacity_, 0);
		size_ = std::exchange(rhs.size_, 0);
	}

	String& String::operator=(String&& other) noexcept {
		String(std::move(other)).swap(*this);
		return *this;
	}

	String& String::operator=(char* buf) {
		size_ = strlen(buf);
		if (capacity_ < size_ + 1) {
			reserve(size_ + 1);
		}
		std::copy(buf, buf + size_ + 1, data_);
		return *this;
	}

	String::~String() {
		if (data_ != nullptr) {
			SOUL_ASSERT(0, capacity_ != 0, "");
			allocator_->deallocate(data_);
		}
	}

	void String::swap(String& rhs) noexcept {
		using std::swap;
		swap(allocator_, rhs.allocator_);
		swap(capacity_, rhs.capacity_);
		swap(size_, rhs.size_);
		swap(data_, rhs.data_);
	}

	void String::reserve(soul_size newCapacity) {
		const auto newData = (char*)allocator_->allocate(newCapacity, alignof(char));
		if (data_ != nullptr) {
			memcpy(newData, data_, capacity_ * sizeof(char));
			allocator_->deallocate(data_);
		}
		data_ = newData;
		capacity_ = newCapacity;
	}

	String& String::appendf(const char* format, ...) {
		va_list args;

		va_start(args, format);
		const soul_size needed = vsnprintf(nullptr, 0, format, args);
		va_end(args);

		if (needed + 1 > capacity_ - size_) {
			const soul_size newCapacity = capacity_ >= (needed + 1) ? 2 * capacity_ : capacity_ + (needed + 1);
			reserve(newCapacity);
		}

		va_start(args, format);
		vsnprintf(data_ + size_, (needed + 1), format, args);
		va_end(args);

		size_ += needed;
		data_[size_] = '\0';

		return *this;
	}


}