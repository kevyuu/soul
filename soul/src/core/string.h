#pragma once

#include "core/type.h"
#include "memory/allocator.h"

namespace Soul {

	class String {

	public:

		String(Memory::Allocator* allocator, uint64 initialCapacity);
		String(Memory::Allocator* allocator, char*);
		String(Memory::Allocator* allocator, const char*);

		String();
		String(char*);
		String(const char*);
		
		String(const String&);
		String(String&&) noexcept;
		String& operator=(String);
		String& operator=(char*);
		~String(); 
		void swap(String&) noexcept;
		friend void swap(String& a, String& b) noexcept { a.swap(b); }

		void reserve(uint64 newCapacity);

		String& appendf(const char* format, ...);

		[[nodiscard]] uint64 capacity() const { return _capacity; }

		[[nodiscard]] uint64 size() const { return _size; }

		[[nodiscard]] char* data() const { return _data; }

	private:
		Memory::Allocator* _allocator;
		uint64 _capacity;
		uint64 _size; // string size, not counting NULL
		char* _data;
	};
}
