#pragma once

#include "core/config.h"
#include "core/type.h"
#include "memory/allocator.h"
#include <iostream>
#include <optional>

namespace soul {

    template <typename T>
    concept bit_block_type = std::unsigned_integral<T>;

	template<bit_block_type BlockType>
	class BitRef
	{
    public:

        BitRef(BlockType* ptr, soul_size bit_index);
        BitRef(const BitRef&) = default;
        BitRef(BitRef&&) = default;
        ~BitRef() = default;

        BitRef& operator=(bool val);
        BitRef& operator=(const BitRef& rhs);
        BitRef& operator=(BitRef&& rhs) noexcept;

        BitRef& operator|=(bool val);
        BitRef& operator&=(bool val);
        BitRef& operator^=(bool val);

        bool operator~() const;
	    operator bool() const;
        BitRef& flip();

	    void operator&() = delete;  // NOLINT(google-runtime-operator)

	private:
        
	    BlockType* bit_block_ = nullptr;
        soul_size bit_index_ = 0;
        BitRef() = default;

        void set_true();
        void set_false();

        BlockType get_mask() const;
		void copy_from(const BitRef& rhs);
        
	};

    template <typename Iterator>
    concept bool_input_iterator = std::input_iterator<Iterator> && std::same_as<std::iter_value_t<Iterator>, bool>;

    struct BitVectorInitDesc
    {
        soul_size size = 0;
        bool val = false;
        soul_size capacity = 0;
    };

	template<bit_block_type BlockType = SOUL_BIT_BLOCK_TYPE_DEFAULT, memory::allocator_type AllocatorType = memory::Allocator>
	class BitVector {
	public:
        using this_type = BitVector<BlockType, AllocatorType>;
        using value_type = bool;
        using reference = BitRef<BlockType>;
        using const_reference = bool;

		explicit BitVector(memory::Allocator* allocator = get_default_allocator());
        explicit BitVector(BitVectorInitDesc init_desc, AllocatorType& allocator = *get_default_allocator());
	    explicit BitVector(soul_size size, AllocatorType& allocator = *get_default_allocator());
        BitVector(soul_size size, const value_type& value, AllocatorType& allocator = *get_default_allocator());
		BitVector(const BitVector& other);
        BitVector(const BitVector& other, AllocatorType& allocator);
        BitVector(BitVector&& other) noexcept;

        template <bool_input_iterator Iterator>
        BitVector(Iterator first, Iterator last, AllocatorType& allocator = *get_default_allocator());

	    BitVector& operator=(const BitVector& other);
		BitVector& operator=(BitVector&& other) noexcept;
		~BitVector();

        void swap(this_type& other) noexcept;
        friend void swap(this_type& a, this_type& b) noexcept { a.swap(b); }

        [[nodiscard]] soul_size capacity() const noexcept;
        [[nodiscard]] soul_size size() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

        void set_allocator(AllocatorType& allocator);
        [[nodiscard]] AllocatorType* get_allocator() const noexcept;

        void resize(soul_size size);
        void reserve(soul_size capacity);

		void clear() noexcept; 
        void cleanup();

        void push_back(bool val);
	    reference push_back();
        void pop_back();
        void pop_back(soul_size count);

        [[nodiscard]] reference front() noexcept;
        [[nodiscard]] const_reference front() const noexcept;

        [[nodiscard]] reference back() noexcept;
        [[nodiscard]] const_reference back() const noexcept;

        [[nodiscard]] reference operator[](soul_size index);
        [[nodiscard]] const_reference operator[](soul_size index) const;

		[[nodiscard]] bool test(soul_size index, bool default_value) const;

		void set(soul_size index, bool value = true);
        void set();
	    void reset();
		
	private:
		AllocatorType* allocator_;
		BlockType* blocks_ = nullptr;
		soul_size size_ = 0;
        soul_size capacity_ = 0;

        static constexpr soul_size BLOCK_BIT_COUNT = sizeof(BlockType) * 8;
        static constexpr soul_size GROWTH_FACTOR = 2;

        reference get_bit_ref(soul_size index);
        [[nodiscard]] bool get_bool(soul_size index) const;
        static soul_size get_new_capacity(soul_size old_capacity);
        void init_reserve(soul_size capacity);
        void init_resize(soul_size size, bool val);
	    [[nodiscard]] static soul_size get_block_count(soul_size size);
		[[nodiscard]] static soul_size get_block_index(soul_size index);
		[[nodiscard]] static soul_size get_block_offset(soul_size index);
	};

    template <bit_block_type ElementType>
    BitRef<ElementType>::BitRef(ElementType* ptr, const soul_size bit_index) :
        bit_block_(ptr), bit_index_(bit_index)
    {}

    template <bit_block_type ElementType>
    BitRef<ElementType>& BitRef<ElementType>::operator=(const bool val)
    {
        if (val)
            set_true();
        else
            set_false();
        return *this;
    }

    template <bit_block_type ElementType>
    BitRef<ElementType>& BitRef<ElementType>::operator=(const BitRef& rhs)  // NOLINT(bugprone-unhandled-self-assignment)
    {
        *this = static_cast<bool>(rhs);
        return *this;
    }

    template <bit_block_type BlockType>
    BitRef<BlockType>& BitRef<BlockType>::operator=(BitRef&& rhs) noexcept
    {
        *this = static_cast<bool>(rhs);
        return *this;
    }

    template <bit_block_type BlockType>
    BitRef<BlockType>& BitRef<BlockType>::operator|=(const bool val)
    {
        if (val)
            set_true();
        return *this;
    }

    template <bit_block_type BlockType>
    BitRef<BlockType>& BitRef<BlockType>::operator&=(const bool val)
    {
        if (!val)
            set_false();
        return *this;
    }

    template <bit_block_type BlockType>
    BitRef<BlockType>& BitRef<BlockType>::operator^=(const bool val)
    {
        if (*this != val)
            set_true();
        else
            set_false();
        return *this;
    }

    template <bit_block_type BlockType>
    bool BitRef<BlockType>::operator~() const
    {
        return !(*this);
    }

    template <bit_block_type ElementType>
    BitRef<ElementType>::operator bool() const
    {
        return static_cast<bool>(*bit_block_ & (static_cast<ElementType>(1) << bit_index_));
    }

    template <bit_block_type BlockType>
    BitRef<BlockType>& BitRef<BlockType>::flip()
    {
        const BlockType mask = get_mask();
        if (*this)
            set_false();
        else
            set_true();
        return *this;
    }

    template <bit_block_type BlockType>
    void BitRef<BlockType>::set_true()
    {
        *bit_block_ |= get_mask();
    }

    template <bit_block_type BlockType>
    void BitRef<BlockType>::set_false()
    {
        *bit_block_ &= ~get_mask();
    }

    template <bit_block_type BlockType>
    BlockType BitRef<BlockType>::get_mask() const
    {
        return static_cast<BlockType>(BlockType(1) << bit_index_);
    }

    template <bit_block_type ElementType>
    void BitRef<ElementType>::copy_from(const BitRef& rhs)
    {
        bit_block_ = rhs.bit_block_;
        bit_index_ = rhs.bit_index_;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    BitVector<BlockType, AllocatorType>::BitVector(memory::Allocator* allocator) :
        allocator_(allocator) {}

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    BitVector<BlockType, AllocatorType>::BitVector(BitVectorInitDesc init_desc, AllocatorType& allocator) :
        allocator_(&allocator), size_(init_desc.size)
    {
        const soul_size capacity = init_desc.size > init_desc.capacity ? init_desc.size : init_desc.capacity;
        const soul_size block_count = get_block_count(capacity);
        blocks_ = allocator_->template allocate_array<BlockType>(block_count);
        capacity_ = block_count * BLOCK_BIT_COUNT;
        std::uninitialized_fill_n(blocks_, get_block_count(init_desc.size), init_desc.val ? ~BlockType(0) : BlockType(0));
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    BitVector<BlockType, AllocatorType>::BitVector(const soul_size size, AllocatorType& allocator) :
        allocator_(&allocator)
    {
        init_resize(size, false);
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    BitVector<BlockType, AllocatorType>::BitVector(const soul_size size, const value_type& value, AllocatorType& allocator) :
        allocator_(&allocator)
    {
        init_resize(size, value);
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    BitVector<BlockType, AllocatorType>::BitVector(const BitVector& other) :
        allocator_(other.allocator_), size_(other.size_)
    {
        init_reserve(other.size_);
        std::uninitialized_copy_n(other.blocks_, get_block_count(other.size_), blocks_);
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    BitVector<BlockType, AllocatorType>::BitVector(const BitVector& other, AllocatorType& allocator) :
        allocator_(&allocator), size_(other.size_)
    {
        init_reserve(other.size_);
        std::uninitialized_copy_n(other.blocks_, get_block_count(other.size_), blocks_);
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    BitVector<BlockType, AllocatorType>::BitVector(BitVector&& other) noexcept:
        allocator_(std::exchange(other.allocator_, nullptr)),
        blocks_(std::exchange(other.blocks_, nullptr)),
        size_(std::exchange(other.size_, 0)),
        capacity_(std::exchange(other.capacity_, 0)) {}

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    template <bool_input_iterator Iterator>
    BitVector<BlockType, AllocatorType>::BitVector(Iterator first, Iterator last, AllocatorType& allocator) :
        allocator_(&allocator)
    {
        if constexpr (std::forward_iterator<Iterator>)
            init_reserve(std::distance(first, last));
        for (; first != last; ++first)
            push_back(*first);
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    BitVector<BlockType, AllocatorType>& BitVector<BlockType, AllocatorType>::operator=(const BitVector& other)  // NOLINT(bugprone-unhandled-self-assignment)
    {
        this_type(other, *allocator_).swap(*this);
        return *this;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    BitVector<BlockType, AllocatorType>& 
    BitVector<BlockType, AllocatorType>::operator=(BitVector&& other) noexcept
    {
        if (this->allocator_ == other.allocator_)
            this_type(std::move(other)).swap(*this);
        else
            this_type(other, *allocator_).swap(*this);
        return *this;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    BitVector<BlockType, AllocatorType>::~BitVector()
    {
        SOUL_ASSERT(0, allocator_ != nullptr || blocks_ == nullptr, "");
        if (allocator_ == nullptr) return;
        cleanup();
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    void BitVector<BlockType, AllocatorType>::swap(this_type& other) noexcept
    {
        using std::swap;
        SOUL_ASSERT(0, allocator_ == other.allocator_, "Cannot swap container with different allocators");
        swap(blocks_, other.blocks_);
        swap(size_, other.size_);
        swap(capacity_, other.capacity_);
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    soul_size BitVector<BlockType, AllocatorType>::capacity() const noexcept
    {
        return capacity_;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    soul_size BitVector<BlockType, AllocatorType>::size() const noexcept
    {
        return size_;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    bool BitVector<BlockType, AllocatorType>::empty() const noexcept
    {
        return size_ == 0;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    void BitVector<BlockType, AllocatorType>::set_allocator(AllocatorType& allocator)
    {
        if (allocator_ != nullptr && blocks_ != nullptr)
        {
            const soul_size block_count = get_block_count(size_);
            BlockType* blocks = allocator.template allocate_array<BlockType>(size_);
            std::uninitialized_move_n(blocks_, block_count, blocks);
            allocator_->deallocate_array(blocks_, get_block_count(capacity_));
            blocks_ = blocks;
        }
        allocator_ = &allocator;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    AllocatorType* BitVector<BlockType, AllocatorType>::get_allocator() const noexcept
    {
        return allocator_;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    void BitVector<BlockType, AllocatorType>::resize(const soul_size size)
    {
        if (size > size_)
        {
            if (size > capacity_) reserve(size);
            const soul_size old_last_block = get_block_index(size_);
            blocks_[old_last_block] &= ((BlockType(1) << get_block_offset(size_)) - 1);
            const soul_size block_count_diff = get_block_count(size) - (old_last_block + 1);
            std::uninitialized_fill_n(blocks_ + old_last_block + 1, block_count_diff, false);
        }
        size_ = size;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    void BitVector<BlockType, AllocatorType>::reserve(const soul_size capacity)
    {
        if (capacity < capacity_) { return; }
        BlockType* old_blocks = blocks_;
        const soul_size block_count = get_block_count(capacity);
        blocks_ = allocator_->template allocate_array<BlockType>(block_count);
        if (old_blocks != nullptr)
        {
            std::uninitialized_move_n(old_blocks, get_block_count(size_), blocks_);
            allocator_->deallocate_array(old_blocks, get_block_count(capacity_));
        }
        capacity_ = block_count * BLOCK_BIT_COUNT;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    void BitVector<BlockType, AllocatorType>::clear() noexcept
    {
        size_ = 0;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    void BitVector<BlockType, AllocatorType>::cleanup()
    {
        clear();
        allocator_->deallocate_array(blocks_, get_block_count(capacity_));
        blocks_ = nullptr;
        capacity_ = 0;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    void BitVector<BlockType, AllocatorType>::push_back(const bool val)
    {
        if (size_ == capacity_) reserve(get_new_capacity(capacity_));
        size_++;
        back() = val;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    typename BitVector<BlockType, AllocatorType>::reference
    BitVector<BlockType, AllocatorType>::push_back()
    {
        if (size_ == capacity_) reserve(get_new_capacity(capacity_));
        size_++;
        back() = false;
        return back();
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    void BitVector<BlockType, AllocatorType>::pop_back()
    {
        SOUL_ASSERT(0, size_ != 0, "");
        size_--;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    void BitVector<BlockType, AllocatorType>::pop_back(const soul_size count)
    {
        SOUL_ASSERT(0, size_ >= count, "");
        size_ -= count;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    typename BitVector<BlockType, AllocatorType>::reference
    BitVector<BlockType, AllocatorType>::front() noexcept
    {
        SOUL_ASSERT(0, size_ != 0, "");
        return reference(blocks_, 0);
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    typename BitVector<BlockType, AllocatorType>::const_reference
    BitVector<BlockType, AllocatorType>::front() const noexcept
    {
        SOUL_ASSERT(0, size_ != 0, "");
        return static_cast<const_reference>(blocks_[0] & BlockType(1));
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    typename BitVector<BlockType, AllocatorType>::reference
    BitVector<BlockType, AllocatorType>::back() noexcept
    {
        SOUL_ASSERT(0, size_ != 0, "");
        return get_bit_ref(size_ - 1);
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    typename BitVector<BlockType, AllocatorType>::const_reference
    BitVector<BlockType, AllocatorType>::back() const noexcept
    {
        SOUL_ASSERT(0, size_ != 0, "");
        return static_cast<const_reference>(blocks_[get_block_index(size_ - 1)] & (BlockType(1) << get_block_offset(size_ - 1)));
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    typename BitVector<BlockType, AllocatorType>::reference
    BitVector<BlockType, AllocatorType>::operator[](const soul_size index)
    {
        SOUL_ASSERT(0, index < size_, "");
        return get_bit_ref(index);
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    typename BitVector<BlockType, AllocatorType>::const_reference
    BitVector<BlockType, AllocatorType>::operator[](const soul_size index) const
    {
        SOUL_ASSERT(0, index < size_, "");
        return get_bool(index);
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    bool BitVector<BlockType, AllocatorType>::test(const soul_size index, const bool default_value) const
    {
        if (index >= size_)
            return default_value;
        return get_bool(index);
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    void BitVector<BlockType, AllocatorType>::set(const soul_size index, const bool value)
    {
        if (index >= size_) resize(index + 1);
        get_bit_ref(index) = value;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    void BitVector<BlockType, AllocatorType>::set()
    {
        std::fill_n(blocks_, get_block_count(size_), ~BlockType(0));
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    void BitVector<BlockType, AllocatorType>::reset()
    {
        std::fill_n(blocks_, get_block_count(size_), BlockType(0));
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    typename BitVector<BlockType, AllocatorType>::reference BitVector<BlockType, AllocatorType>::get_bit_ref(
        const soul_size index)
    {
        return reference(blocks_ + get_block_index(index), get_block_offset(index));
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    bool BitVector<BlockType, AllocatorType>::get_bool(const soul_size index) const
    {
        return blocks_[get_block_index(index)] & (BlockType(1) << get_block_offset(index));
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    soul_size BitVector<BlockType, AllocatorType>::get_new_capacity(const soul_size old_capacity)
    {
        return old_capacity * GROWTH_FACTOR + BLOCK_BIT_COUNT;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    void BitVector<BlockType, AllocatorType>::init_reserve(const soul_size capacity)
    {
        const soul_size block_count = get_block_count(capacity);
        blocks_ = allocator_->template allocate_array<BlockType>(block_count);
        capacity_ = block_count * BLOCK_BIT_COUNT;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    void BitVector<BlockType, AllocatorType>::init_resize(const soul_size size, const bool val)
    {
        const soul_size block_count = get_block_count(size);
        blocks_ = allocator_->template allocate_array<BlockType>(block_count);
        capacity_ = block_count * BLOCK_BIT_COUNT;
        std::uninitialized_fill_n(blocks_, block_count, val ? ~BlockType(0) : BlockType(0));
        size_ = size;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    soul_size BitVector<BlockType, AllocatorType>::get_block_count(const soul_size size)
    {
        return (size + BLOCK_BIT_COUNT - 1) / BLOCK_BIT_COUNT;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    soul_size BitVector<BlockType, AllocatorType>::get_block_index(const soul_size index)
    {
        return index / BLOCK_BIT_COUNT;
    }

    template <bit_block_type BlockType, memory::allocator_type AllocatorType>
    soul_size BitVector<BlockType, AllocatorType>::get_block_offset(const soul_size index)
    {
        return index % BLOCK_BIT_COUNT;
    }
} // namespace soul