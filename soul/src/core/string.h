#pragma once

#include "core/compiler.h"
#include "core/type.h"
#include "memory/allocator.h"

namespace soul {

	class String {

	public:

		String(memory::Allocator* allocator, uint64 initialCapacity);
		String(memory::Allocator* allocator, char*);
		String(memory::Allocator* allocator, const char*);

		String();
		String(char*);
		String(const char*);
		
		String(const String&);
		String& operator=(const String&);
		String(String&&) noexcept;
		String& operator=(String&&) noexcept;
		String& operator=(char*);
		~String(); 
		void swap(String&) noexcept;
		friend void swap(String& a, String& b) noexcept { a.swap(b); }

		void reserve(soul_size newCapacity);

		String& appendf(const char* format, ...);

		[[nodiscard]] soul_size capacity() const { return capacity_; }

		[[nodiscard]] soul_size size() const { return size_; }

		[[nodiscard]] const char* data() const { return data_; }

	private:
		memory::Allocator* allocator_;
		soul_size capacity_;
		soul_size size_; // string size, not counting NULL
		char* data_;
	};
}
