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
		BitSet(BitSet&& other) noexcept;
		BitSet& operator=(BitSet&& other) noexcept;
		~BitSet();

		void clear() noexcept;
		[[nodiscard]] bool test(soul_size index) const;
		void set(soul_size index);
		void unset(soul_size index);
		void resize(soul_size size);
		[[nodiscard]] soul_size size() const noexcept;
		void cleanup();

	private:
	    using BitUnit = uint8;
		Memory::Allocator* _allocator;
		BitUnit* _bitTable;
		soul_size _size;

		static constexpr soul_size BIT_TABLE_SHIFT = 3;
		static_assert(sizeof(*_bitTable) * 8 == 1 << BIT_TABLE_SHIFT);

		static soul_size ByteCount(soul_size size);
		static soul_size TableIndex(soul_size index);
		static soul_size TableOffset(soul_size index);
	};
}