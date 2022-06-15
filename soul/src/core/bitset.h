#pragma once

#include "core/compiler.h"
#include "core/config.h"
#include "core/type.h"
#include "memory/allocator.h"

namespace soul {

	class BitSet {
	public:
		explicit BitSet(memory::Allocator* allocator = get_default_allocator());
		BitSet(const BitSet& other);
		BitSet& operator=(const BitSet& other);
		BitSet(BitSet&& other) noexcept;
		BitSet& operator=(BitSet&& other) noexcept;
		~BitSet();

		void clear() noexcept;
		SOUL_NODISCARD bool test(soul_size index) const;
		void set(soul_size index);
		void unset(soul_size index);
		void resize(soul_size size);
		SOUL_NODISCARD soul_size size() const noexcept;
		void cleanup();

	private:
	    using BitUnit = uint8;
		memory::Allocator* allocator_;
		BitUnit* bit_table_ = nullptr;
		soul_size size_ = 0;

		static constexpr soul_size BIT_TABLE_SHIFT = 3;
		static_assert(sizeof(*bit_table_) * 8 == 1 << BIT_TABLE_SHIFT);

		SOUL_NODISCARD static soul_size byte_count(soul_size size);
		SOUL_NODISCARD static soul_size table_index(soul_size index);
		SOUL_NODISCARD static soul_size table_offset(soul_size index);
	};
}