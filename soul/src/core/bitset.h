#pragma once
#include "core/type.h"
#include "memory/allocator.h"

namespace Soul {
	class BitSet {
	public:

		BitSet();
		explicit BitSet(Memory::Allocator* allocator);
		BitSet(const BitSet& other);
		BitSet& operator=(const BitSet& other);
		BitSet(BitSet&& other);
		BitSet& operator=(BitSet&& other);
		~BitSet();

		void clear();
		bool test(uint32 index) const;
		void set(uint32 index);
		void unset(uint32 index);
		void resize(uint32 size);
		uint32 size() const;
		void cleanup();

	private:
	    using BitUnit = uint8;
		Memory::Allocator* _allocator;
		BitUnit* _bitTable;
		uint32 _size;

		static const uint32 _BIT_TABLE_SHIFT = 3;
		static_assert(sizeof(*_bitTable) * 8 == 1 << _BIT_TABLE_SHIFT, "");
		static uint32 _ByteCount(uint32 size);
		static uint32 _TableIndex(uint32 index);
		static uint32 _TableOffset(uint32 index);
	};
}