#pragma once

#include "core/config.h"
#include "core/type.h"
#include "memory/allocator.h"

namespace soul {

	class CString {

	public:

		using this_type = CString;
		using value_type = char;
		using pointer = char*;
		using const_pointer = const char*;
		using reference = char&;
		using const_reference = const char&;

		explicit CString(soul_size size, memory::Allocator& allocator = *get_default_allocator());
		CString(char*, memory::Allocator& allocator);
		CString(const char*, memory::Allocator& allocator);

		explicit CString(memory::Allocator* allocator = get_default_allocator());
		CString(char*);
		CString(const char*);
		
		CString(const CString&);
		CString& operator=(const CString&);
		CString(CString&&) noexcept;
		CString& operator=(CString&&) noexcept;
		CString& operator=(char*);
		~CString(); 
		void swap(CString&) noexcept;
		friend void swap(CString& a, CString& b) noexcept { a.swap(b); }

		void reserve(soul_size new_capacity);

		CString& append(const CString& x);
		CString& appendf(SOUL_FORMAT_STRING const char* format, ...);

		[[nodiscard]] soul_size capacity() const { return capacity_; }
		[[nodiscard]] soul_size size() const { return size_; }

		[[nodiscard]] pointer data() { return data_; }
		[[nodiscard]] const_pointer data() const { return data_; }

	private:
		memory::Allocator* allocator_;
		soul_size capacity_ = 0;
		soul_size size_ = 0; // string size, not counting NULL
		char* data_ = nullptr;
	};
}
