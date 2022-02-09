#include "core/bitset.h"
#include "runtime/runtime.h"

namespace soul {
	soul_size BitSet::byte_count(soul_size size) {
		return ((size + sizeof(BitUnit) - 1) >> BIT_TABLE_SHIFT) * sizeof(BitUnit);
	}

	soul_size BitSet::table_index(soul_size index) {
		return index >> BIT_TABLE_SHIFT;
	}

	soul_size BitSet::table_offset(soul_size index) {
		return index % 8;
	}

	BitSet::BitSet(memory::Allocator* allocator) : allocator_(allocator) {}

	BitSet::BitSet(const BitSet& other) : allocator_(other.allocator_) {
		size_ = other.size_;
		bit_table_ = (uint8*) allocator_->allocate(byte_count(size_), alignof(BitUnit));
		memcpy(bit_table_, other.bit_table_, byte_count(size_));
	}

	BitSet& BitSet::operator=(const BitSet& other) {
		if (this == &other) { return *this; }
		allocator_->deallocate(bit_table_, byte_count(size_));
		size_ = other.size_;
		bit_table_ = (uint8*) allocator_->allocate(byte_count(size_), alignof(BitUnit));
		memcpy(bit_table_, other.bit_table_, byte_count(size_));
		return *this;
	}

	BitSet::BitSet(BitSet&& other) noexcept {
		allocator_ = other.allocator_;
		bit_table_ = other.bit_table_;
		size_ = other.size_;

		other.bit_table_ = nullptr;
		other.size_ = 0;
	}

	BitSet& BitSet::operator=(BitSet&& other) noexcept {
		allocator_->deallocate(bit_table_, byte_count(size_));
		new (this) BitSet(std::move(other));
		return *this;
	}

	BitSet::~BitSet() {
		allocator_->deallocate(bit_table_, byte_count(size_));
	}

	void BitSet::clear() noexcept
	{
		memset(bit_table_, 0, byte_count(size_));
	}

	bool BitSet::test(soul_size index) const {
		SOUL_ASSERT(0, index < size_, "");
		return bit_table_[table_index(index)] & (1 << table_offset(index));
	}

	void BitSet::set(soul_size index) {
		SOUL_ASSERT(0, index < size_, "");
		bit_table_[table_index(index)] |= (1 << table_offset(index));
	}

	void BitSet::unset(soul_size index) {
		SOUL_ASSERT(0, index < size_, "");
		bit_table_[table_index(index)] &= (~(1 << table_offset(index)));
	}

	void BitSet::resize(soul_size size) {
		uint8* oldBitTable = bit_table_;
		bit_table_ = (uint8*) allocator_->allocate(byte_count(size), alignof(BitUnit));

		memcpy(bit_table_, oldBitTable, std::min(byte_count(size), byte_count(size_)));

		allocator_->deallocate(oldBitTable, byte_count(size_));
		size_ = size;
	}

	soul_size BitSet::size() const noexcept {return size_;}

	void BitSet::cleanup() {
		allocator_->deallocate(bit_table_, byte_count(size_));
		bit_table_ = nullptr;
		size_ = 0;
	}
};