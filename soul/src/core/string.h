#pragma once

#include "core/compiler.h"
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
		String& operator=(const String&);
		String(String&&) noexcept;
		String& operator=(String&&) noexcept;
		String& operator=(char*);
		~String(); 
		void swap(String&) noexcept;
		friend void swap(String& a, String& b) noexcept { a.swap(b); }

		void reserve(soul_size newCapacity);

		String& appendf(const char* format, ...);

		SOUL_NODISCARD soul_size capacity() const { return _capacity; }

		SOUL_NODISCARD soul_size size() const { return _size; }

		SOUL_NODISCARD char* data() const { return _data; }

	private:
		Memory::Allocator* _allocator;
		soul_size _capacity;
		soul_size _size; // string size, not counting NULL
		char* _data;
	};
}