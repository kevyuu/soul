#include "core/bitset.h"
#include "runtime/runtime.h"

namespace Soul {
	soul_size BitSet::ByteCount(soul_size size) {
		return ((size + sizeof(BitUnit) - 1) >> BIT_TABLE_SHIFT) * sizeof(BitUnit);
	}

	soul_size BitSet::TableIndex(soul_size index) {
		return index >> BIT_TABLE_SHIFT;
	}

	soul_size BitSet::TableOffset(soul_size index) {
		return index % 8;
	}

	BitSet::BitSet(Memory::Allocator* allocator) : _allocator(allocator) {}

	BitSet::BitSet(const BitSet& other) : _allocator(other._allocator) {
		_size = other._size;
		_bitTable = (uint8*) _allocator->allocate(ByteCount(_size), alignof(BitUnit));
		memcpy(_bitTable, other._bitTable, ByteCount(_size));
	}

	BitSet& BitSet::operator=(const BitSet& other) {
		if (this == &other) { return *this; }
		_allocator->deallocate(_bitTable, ByteCount(_size));
		_size = other._size;
		_bitTable = (uint8*) _allocator->allocate(ByteCount(_size), alignof(BitUnit));
		memcpy(_bitTable, other._bitTable, ByteCount(_size));
		return *this;
	}

	BitSet::BitSet(BitSet&& other) noexcept {
		_allocator = other._allocator;
		_bitTable = other._bitTable;
		_size = other._size;

		other._bitTable = nullptr;
		other._size = 0;
	}

	BitSet& BitSet::operator=(BitSet&& other) noexcept {
		_allocator->deallocate(_bitTable, ByteCount(_size));
		new (this) BitSet(std::move(other));
		return *this;
	}

	BitSet::~BitSet() {
		_allocator->deallocate(_bitTable, ByteCount(_size));
	}

	void BitSet::clear() noexcept
	{
		memset(_bitTable, 0, ByteCount(_size));
	}

	bool BitSet::test(soul_size index) const {
		SOUL_ASSERT(0, index < _size, "");
		return _bitTable[TableIndex(index)] & (1 << TableOffset(index));
	}

	void BitSet::set(soul_size index) {
		SOUL_ASSERT(0, index < _size, "");
		_bitTable[TableIndex(index)] |= (1 << TableOffset(index));
	}

	void BitSet::unset(soul_size index) {
		SOUL_ASSERT(0, index < _size, "");
		_bitTable[TableIndex(index)] &= (~(1 << TableOffset(index)));
	}

	void BitSet::resize(soul_size size) {
		uint8* oldBitTable = _bitTable;
		_bitTable = (uint8*) _allocator->allocate(ByteCount(size), alignof(BitUnit));

		memcpy(_bitTable, oldBitTable, std::min(ByteCount(size), ByteCount(_size)));

		_allocator->deallocate(oldBitTable, ByteCount(_size));
		_size = size;
	}

	soul_size BitSet::size() const noexcept {return _size;}

	void BitSet::cleanup() {
		_allocator->deallocate(_bitTable, ByteCount(_size));
		_bitTable = nullptr;
		_size = 0;
	}
};