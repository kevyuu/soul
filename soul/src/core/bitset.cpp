#include "core/bitset.h"
#include "core/math.h"

#include "memory/memory.h"

namespace Soul {

	uint32 BitSet::_ByteCount(uint32 size) {
		return ((size + sizeof(*_bitTable) - 1) >> _BIT_TABLE_SHIFT) * sizeof(*_bitTable);
	}

	uint32 BitSet::_TableIndex(uint32 index) {
		return index >> _BIT_TABLE_SHIFT;
	}

	uint32 BitSet::_TableOffset(uint32 index) {
		return index % 8;
	}

	BitSet::BitSet() : _allocator((Memory::Allocator*)Memory::GetContextAllocator()), _bitTable(nullptr), _size(0) {}
	BitSet::BitSet(Memory::Allocator* allocator) : _allocator(allocator), _bitTable(nullptr), _size(0) {}

	BitSet::BitSet(const BitSet& other) : _allocator(other._allocator) {
		_size = other._size;
		_bitTable = (uint8*) _allocator->allocate(_ByteCount(_size), alignof(*_bitTable));
		memcpy(_bitTable, other._bitTable, _ByteCount(_size));
	}

	BitSet& BitSet::operator=(const BitSet& other) {
		if (this == &other) { return *this; }
		_allocator->deallocate(_bitTable, _ByteCount(_size));
		_size = other._size;
		_bitTable = (uint8*) _allocator->allocate(_ByteCount(_size), alignof(*_bitTable));
		memcpy(_bitTable, other._bitTable, _ByteCount(_size));
	}

	BitSet::BitSet(BitSet&& other) {
		_allocator = other._allocator;
		_bitTable = other._bitTable;
		_size = other._size;

		other._bitTable = nullptr;
		other._size = 0;
	}

	BitSet& BitSet::operator=(BitSet&& other) {
		_allocator->deallocate(_bitTable, _ByteCount(_size));
		new (this) BitSet(std::move(other));
		return *this;
	}

	BitSet::~BitSet() {
		_allocator->deallocate(_bitTable, _ByteCount(_size));
	}

	void BitSet::clear() {
		memset(_bitTable, 0, _ByteCount(_size));
	}

	bool BitSet::test(uint32 index) const {
		SOUL_ASSERT(0, index < _size, "");
		return _bitTable[_TableIndex(index)] & (1 << _TableOffset(index));
	}

	void BitSet::set(uint32 index) {
		SOUL_ASSERT(0, index < _size, "");
		_bitTable[_TableIndex(index)] |= (1 << _TableOffset(index));
	}

	void BitSet::unset(uint32 index) {
		SOUL_ASSERT(0, index < _size, "");
		_bitTable[_TableIndex(index)] &= (~(1 << _TableOffset(index)));
	}

	void BitSet::resize(uint32 size) {
		uint8* oldBitTable = _bitTable;
		_bitTable = (uint8*) _allocator->allocate(_ByteCount(size), alignof(*_bitTable));

		memcpy(_bitTable, oldBitTable, min(_ByteCount(size), _ByteCount(_size)));

		_allocator->deallocate(oldBitTable, _ByteCount(_size));
		_size = size;
	}

	uint32 BitSet::size() const {return _size;}

	void BitSet::cleanup() {
		_allocator->deallocate(_bitTable, _ByteCount(_size));
		_bitTable = nullptr;
		_size = 0;
	}
};