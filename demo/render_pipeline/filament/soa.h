#pragma once

#include "core/type.h"
#include "core/util.h"
#include "memory/allocator.h"

namespace soul_fila
{
    template <typename EnumIndex, typename ... Elements>
    class SoaPool {
        // number of elements
        static constexpr const size_t kArrayCount = sizeof...(Elements);

    public:
        using SoA = SoaPool<Elements ...>;

        // Type of the Nth array
        template<size_t N>
        using TypeAt = typename std::tuple_element<N, std::tuple<Elements...>>::type;

        // Number of arrays
        static constexpr size_t getArrayCount() noexcept { return kArrayCount; }

        // Size needed to store "size" array elements
        static size_t getNeededSize(size_t size) noexcept {
            return getOffset(kArrayCount - 1, size) + sizeof(TypeAt<kArrayCount - 1>) * size;
        }

        // --------------------------------------------------------------------------------------------

        class Structure;
        template<typename T> class Iterator;
        using iterator = Iterator<SoaPool*>;
        using const_iterator = Iterator<SoaPool const*>;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        /*
         * An object that represents a reference to the type dereferenced by iterator.
         * In other words, it's the return type of iterator::operator*(), and since it
         * cannot be a C++ reference (&), it's an object that acts like it.
         */
        class StructureRef {
            friend class Structure;
            friend iterator;
            friend const_iterator;
            SoaPool* const SOUL_RESTRICT soa;
            size_t const index;

            StructureRef(SoaPool* soa, size_t index) : soa(soa), index(index) { }

            // assigns a value_type to a reference (i.e. assigns to what's pointed to by the reference)
            template<size_t ... Is>
            StructureRef& assign(Structure const& rhs, std::index_sequence<Is...>);

            // assigns a value_type to a reference (i.e. assigns to what's pointed to by the reference)
            template<size_t ... Is>
            StructureRef& assign(Structure&& rhs, std::index_sequence<Is...>) noexcept;

            // objects pointed to by reference can be swapped, so provide the special swap() function.
            friend void swap(StructureRef lhs, StructureRef rhs) {
                lhs.soa->swap(lhs.index, rhs.index);
            }

        public:
            // references can be created by copy-assignment only
            StructureRef(StructureRef const& rhs) noexcept : soa(rhs.soa), index(rhs.index) { }

            // copy the content of a reference to the content of this one
            StructureRef& operator=(StructureRef const& rhs);

            // move the content of a reference to the content of this one
            StructureRef& operator=(StructureRef&& rhs) noexcept;

            // copy a value_type to the content of this reference
            StructureRef& operator=(Structure const& rhs) {
                return assign(rhs, std::make_index_sequence<kArrayCount>());
            }

            // move a value_type to the content of this reference
            StructureRef& operator=(Structure&& rhs) noexcept {
                return assign(rhs, std::make_index_sequence<kArrayCount>());
            }

            // access the elements of this reference (i.e. the "fields" of the structure)
            template<EnumIndex enumIndex> TypeAt<soul::to_underlying(enumIndex)> const& get() const { return soa->elementAt<enumIndex>(index); }
            template<EnumIndex enumIndex> TypeAt<soul::to_underlying(enumIndex)>& get() { return soa->elementAt<enumIndex>(index); }
        };


        /*
         * The value_type of iterator. This is basically the "structure" of the SoA.
         * Internally we're using a tuple<> to store the data.
         * This object is not trivial to construct, as it copies an entry of the SoA.
         */
        class Structure {
            friend class StructureRef;
            friend iterator;
            friend const_iterator;
            using Type = std::tuple<typename std::decay<Elements>::type...>;
            Type elements;

            template<size_t ... Is>
            static Type init(StructureRef const& rhs, std::index_sequence<Is...>) {
                return Type{ rhs.soa->template elementAt<EnumIndex(Is)>(rhs.index)... };
            }

            template<size_t ... Is>
            static Type init(StructureRef&& rhs, std::index_sequence<Is...>) noexcept {
                return Type{ std::move(rhs.soa->template elementAt<EnumIndex(Is)>(rhs.index))... };
            }

        public:
            Structure(Structure const& rhs) = default;
            Structure(Structure&& rhs) noexcept = default;
            Structure& operator=(Structure const& rhs) = default;
            Structure& operator=(Structure&& rhs) noexcept = default;

            // initialize and assign from a StructureRef
            Structure(StructureRef const& rhs)
                : elements(init(rhs, std::make_index_sequence<kArrayCount>())) {}
            Structure(StructureRef&& rhs) noexcept
                : elements(init(rhs, std::make_index_sequence<kArrayCount>())) {}
            Structure& operator=(StructureRef const& rhs) { return operator=(Structure(rhs)); }
            Structure& operator=(StructureRef&& rhs) noexcept { return operator=(Structure(rhs)); }

            // access the elements of this value_Type (i.e. the "fields" of the structure)
            template<EnumIndex enumIndex> TypeAt<soul::to_underlying(enumIndex)> const& get() const { return std::get<soul::to_underlying(enumIndex)>(elements); }
            template<EnumIndex enumIndex> TypeAt<soul::to_underlying(enumIndex)>& get() { return std::get<soul::to_underlying(enumIndex)>(elements); }
        };


        /*
         * An iterator to the SoA. This is only intended to be used with STL's algorithm, e.g.: sort().
         * Normally, SoA is not iterated globally, but rather an array at a time.
         * Iterating itself is not too costly, as well as dereferencing by reference. However,
         * dereferencing by value is.
         */
        template<typename CVQualifiedSOAPointer>
        class Iterator {
            friend class SoaPool;
            CVQualifiedSOAPointer soa; // don't use restrict, can have aliases if multiple iterators are created
            size_t index;

            Iterator(CVQualifiedSOAPointer soa, size_t index) : soa(soa), index(index) {}

        public:
            using value_type = Structure;
            using reference = StructureRef;
            using pointer = StructureRef*;    // FIXME: this should be a StructurePtr type
            using difference_type = ptrdiff_t;
            using iterator_category = std::random_access_iterator_tag;

            Iterator(Iterator const& rhs) noexcept = default;
            Iterator& operator=(Iterator const& rhs) = default;

            reference operator*() const { return { soa, index }; }
            reference operator*() { return { soa, index }; }
            reference operator[](size_t n) { return *(*this + n); }

            template<EnumIndex enumIndex> TypeAt<soul::to_underlying(enumIndex)> const& get() const { return soa->template elementAt<enumIndex>(index); }
            template<EnumIndex enumIndex> TypeAt<soul::to_underlying(enumIndex)>& get() { return soa->template elementAt<enumIndex>(index); }

            Iterator& operator++() { ++index; return *this; }
            Iterator& operator--() { --index; return *this; }
            Iterator& operator+=(size_t n) { index += n; return *this; }
            Iterator& operator-=(size_t n) { index -= n; return *this; }
            Iterator operator+(size_t n) const { return { soa, index + n }; }
            Iterator operator-(size_t n) const { return { soa, index - n }; }
            difference_type operator-(Iterator const& rhs) const { return index - rhs.index; }
            bool operator==(Iterator const& rhs) const { return (index == rhs.index); }
            bool operator!=(Iterator const& rhs) const { return (index != rhs.index); }
            bool operator>=(Iterator const& rhs) const { return (index >= rhs.index); }
            bool operator> (Iterator const& rhs) const { return (index > rhs.index); }
            bool operator<=(Iterator const& rhs) const { return (index <= rhs.index); }
            bool operator< (Iterator const& rhs) const { return (index < rhs.index); }

            // Postfix operator needed by Microsoft STL.
            const Iterator operator++(int) { Iterator it(*this); index++; return it; }
            const Iterator operator--(int) { Iterator it(*this); index--; return it; }
        };

        iterator begin() noexcept { return { this, 0u }; }
        iterator end() noexcept { return { this, mSize }; }
        const_iterator begin() const noexcept { return { this, 0u }; }
        const_iterator end() const noexcept { return { this, mSize }; }

        // --------------------------------------------------------------------------------------------

        explicit SoaPool(soul::memory::Allocator* allocator = soul::runtime::get_context_allocator()) : mAllocator(allocator) {}

        SoaPool(soul_size capacity, soul::memory::Allocator* allocator = soul::runtime::get_context_allocator()) : mAllocator(allocator) {
            setCapacity(capacity);
        }

        // not copyable for now
        SoaPool(SoaPool const& rhs) = delete;
        SoaPool& operator=(SoaPool const& rhs) = delete;

        // movability is trivial, so support it
        SoaPool(SoaPool&& rhs) noexcept {
            using std::swap;
            swap(mCapacity, rhs.mCapacity);
            swap(mSize, rhs.mSize);
            swap(mArrayOffset, rhs.mArrayOffset);
            swap(mAllocator, rhs.mAllocator);
        }

        SoaPool& operator=(SoaPool&& rhs) noexcept {
            if (this != &rhs) {
                using std::swap;
                swap(mCapacity, rhs.mCapacity);
                swap(mSize, rhs.mSize);
                swap(mArrayOffset, rhs.mArrayOffset);
                swap(mAllocator, rhs.mAllocator);
            }
            return *this;
        }

        ~SoaPool() {
            if (mAllocator == nullptr)
            {
                SOUL_ASSERT(0, mSize == 0, "");
                SOUL_ASSERT(0, mCapacity == 0, "");
                return;
            }
            destroy_each(0, mSize);
            mAllocator->deallocate(mArrayOffset[0], getNeededSize(mCapacity));
        }

        // --------------------------------------------------------------------------------------------

        // return the size the array
        size_t size() const noexcept {
            return mSize;
        }

        // return the capacity of the array
        size_t capacity() const noexcept {
            return mCapacity;
        }

        // set the capacity of the array. the capacity cannot be smaller than the current size,
        // the call is a no-op in that case.
        SOUL_NOINLINE
            void setCapacity(size_t capacity) {
            // allocate enough space for "capacity" elements of each array
            // capacity cannot change when optional storage is specified
            if (capacity >= mSize) {
                const size_t sizeNeeded = getNeededSize(capacity);
                void* buffer = mAllocator->allocate(sizeNeeded, alignof(void*));

                // move all the items (one array at a time) from the old allocation to the new
                // this also update the array pointers
                move_each(buffer, capacity);

                // free the old buffer
                std::swap(buffer, mArrayOffset[0]);
                mAllocator->deallocate(buffer, getNeededSize(mCapacity));

                // and make sure to update the capacity
                mCapacity = capacity;
            }
        }

        void ensureCapacity(size_t needed) {
            if (SOUL_UNLIKELY(needed > mCapacity)) {
                // not enough space, increase the capacity
                const size_t capacity = (needed * 3 + 1) / 2;
                setCapacity(capacity);
            }
        }

        // grow or shrink the array to the given size. When growing, new elements are constructed
        // with their default constructor. when shrinking, discarded elements are destroyed.
        // If the arrays don't have enough capacity, the capacity is increased accordingly
        // (the capacity is set to 3/2 of the asked size).
        SOUL_NOINLINE
            void resize(size_t needed) {
            ensureCapacity(needed);
            resizeNoCheck(needed);
            if (needed <= mCapacity) {
                // TODO: see if we should shrink the arrays
            }
        }

        void clear() noexcept {
            resizeNoCheck(0);
        }


        inline void swap(size_t i, size_t j) noexcept {
            forEach([i, j](auto p) {
                using std::swap;
                swap(p[i], p[j]);
                });
        }

        // remove and destroy the last element of each array
        inline void pop_back() noexcept {
            if (mSize) {
                destroy_each(mSize - 1, mSize);
                mSize--;
            }
        }

        // create an element at the end of each array
        SoaPool& push_back() noexcept {
            resize(mSize + 1);
            return *this;
        }

        SoaPool& push_back(Elements const& ... args) noexcept {
            ensureCapacity(mSize + 1);
            return push_back_unsafe(args...);
        }

        SoaPool& push_back(Elements&& ... args) noexcept {
            ensureCapacity(mSize + 1);
            return push_back_unsafe(std::forward<Elements>(args)...);
        }

        SoaPool& push_back_unsafe(Elements const& ... args) noexcept {
            const size_t last = mSize++;
            size_t i = 0;
            int SOUL_UNUSED dummy[] = {
                    (new(getArray<Elements>(i) + last)Elements(args), i++, 0)... };
            return *this;
        }

        SoaPool& push_back_unsafe(Elements&& ... args) noexcept {
            const size_t last = mSize++;
            size_t i = 0;
            int SOUL_UNUSED dummy[] = {
                    (new(getArray<Elements>(i) + last)Elements(std::forward<Elements>(args)), i++, 0)... };
            return *this;
        }

        template<typename F, typename ... ARGS>
        void forEach(F&& f, ARGS&& ... args) {
            size_t i = 0;
            int SOUL_UNUSED dummy[] = {
                    (f(getArray<Elements>(i), std::forward<ARGS>(args)...), i++, 0)... };
        }

        // return a pointer to the first element of the ElementIndex]th array
        template<size_t ElementIndex>
        TypeAt<ElementIndex>* dataImpl() noexcept {
            return getArray<TypeAt<ElementIndex>>(ElementIndex);
        }

        template<size_t ElementIndex>
        TypeAt<ElementIndex> const* dataImpl() const noexcept {
            return getArray<TypeAt<ElementIndex>>(ElementIndex);
        }

        // return a pointer to the first element of the ElementIndex]th array
        template<EnumIndex enumIndex>
        TypeAt<soul::to_underlying(enumIndex)>* data() noexcept {
            return dataImpl<soul::to_underlying(enumIndex)>();
        }

        template<EnumIndex enumIndex>
        TypeAt<soul::to_underlying(enumIndex)> const* data() const noexcept {
            return dataImpl<soul::to_underlying(enumIndex)>();
        }


        template<size_t ElementIndex>
        TypeAt<ElementIndex>* begin() noexcept {
            return getArray<TypeAt<ElementIndex>>(ElementIndex);
        }

        template<size_t ElementIndex>
        TypeAt<ElementIndex> const* begin() const noexcept {
            return getArray<TypeAt<ElementIndex>>(ElementIndex);
        }

        template<size_t ElementIndex>
        TypeAt<ElementIndex>* end() noexcept {
            return getArray<TypeAt<ElementIndex>>(ElementIndex) + size();
        }

        template<size_t ElementIndex>
        TypeAt<ElementIndex> const* end() const noexcept {
            return getArray<TypeAt<ElementIndex>>(ElementIndex) + size();
        }
        
        // return a reference to the index'th element of the ElementIndex'th array
        template<EnumIndex enumIndex>
        TypeAt<soul::to_underlying(enumIndex)>& elementAt(soul_size index) noexcept {
            return dataImpl<soul::to_underlying(enumIndex)>()[index];
        }

        // return a reference to the index'th element of the ElementIndex'th array
        template<EnumIndex enumIndex>
        TypeAt<soul::to_underlying(enumIndex)> const& elementAt(soul_size index) const noexcept {
            return dataImpl<soul::to_underlying(enumIndex)>()[index];
        }

        // return a reference to the last element of the ElementIndex'th array
        template<size_t ElementIndex>
        TypeAt<ElementIndex>& back() noexcept {
            return dataImpl<ElementIndex>()[size() - 1];
        }

        template<size_t ElementIndex>
        TypeAt<ElementIndex> const& back() const noexcept {
            return dataImpl<ElementIndex>()[size() - 1];
        }

        template <size_t E>
        struct Field {
            SoA& soa;
            uint32 i;
            using Type = typename SoA::template TypeAt<E>;

            SOUL_ALWAYS_INLINE Field& operator = (Field&& rhs) noexcept {
                soa.elementAt<E>(i) = soa.elementAt<E>(rhs.i);
                return *this;
            }

            // auto-conversion to the field's type
            SOUL_ALWAYS_INLINE operator Type& () noexcept {
                return soa.elementAt<E>(i);
            }
            SOUL_ALWAYS_INLINE operator Type const& () const noexcept {
                return soa.elementAt<E>(i);
            }
            // dereferencing the selected field
            SOUL_ALWAYS_INLINE Type& operator ->() noexcept {
                return soa.elementAt<E>(i);
            }
            SOUL_ALWAYS_INLINE Type const& operator ->() const noexcept {
                return soa.elementAt<E>(i);
            }
            // address-of the selected field
            SOUL_ALWAYS_INLINE Type* operator &() noexcept {
                return &soa.elementAt<E>(i);
            }
            SOUL_ALWAYS_INLINE Type const* operator &() const noexcept {
                return &soa.elementAt<E>(i);
            }
            // assignment to the field
            SOUL_ALWAYS_INLINE Type const& operator = (Type const& other) noexcept {
                return (soa.elementAt<E>(i) = other);
            }
            SOUL_ALWAYS_INLINE Type const& operator = (Type&& other) noexcept {
                return (soa.elementAt<E>(i) = other);
            }
            // comparisons
            SOUL_ALWAYS_INLINE bool operator==(Type const& other) const {
                return (soa.elementAt<E>(i) == other);
            }
            SOUL_ALWAYS_INLINE bool operator!=(Type const& other) const {
                return (soa.elementAt<E>(i) != other);
            }
            // calling the field
            template <typename ... ARGS>
            SOUL_ALWAYS_INLINE decltype(auto) operator()(ARGS&& ... args) noexcept {
                return soa.elementAt<E>(i)(std::forward<ARGS>(args)...);
            }
            template <typename ... ARGS>
            SOUL_ALWAYS_INLINE decltype(auto) operator()(ARGS&& ... args) const noexcept {
                return soa.elementAt<E>(i)(std::forward<ARGS>(args)...);
            }
        };

    private:
        template<typename T>
        T const* getArray(size_t arrayIndex) const {
            return static_cast<T const*>(mArrayOffset[arrayIndex]);
        }

        template<typename T>
        T* getArray(size_t arrayIndex) {
            return static_cast<T*>(mArrayOffset[arrayIndex]);
        }

        inline void resizeNoCheck(size_t needed) noexcept {
            assert(mCapacity >= needed);
            if (needed < mSize) {
                // we shrink the arrays
                destroy_each(needed, mSize);
            }
            else if (needed > mSize) {
                // we grow the arrays
                construct_each(mSize, needed);
            }
            // record the new size of the arrays
            mSize = needed;
        }

        // this calculate the offset adjusted for all data alignment of a given array
        static inline size_t getOffset(size_t index, size_t capacity) noexcept {
            auto offsets = getOffsets(capacity);
            return offsets[index];
        }

        static inline std::array<size_t, kArrayCount> getOffsets(size_t capacity) noexcept {
            // compute the required size of each array
            const size_t sizes[] = { (sizeof(Elements) * capacity)... };

            // we align each array to the same alignment guaranteed by malloc
            const size_t align = alignof(std::max_align_t);

            // hopefully most of this gets unrolled and inlined
            std::array<size_t, kArrayCount> offsets;
            offsets[0] = 0;
#pragma unroll
            for (size_t i = 1; i < kArrayCount; i++) {
                size_t unalignment = sizes[i - 1] % align;
                size_t alignment = unalignment ? (align - unalignment) : 0;
                offsets[i] = offsets[i - 1] + (sizes[i - 1] + alignment);
            }
            return offsets;
        }

        void construct_each(size_t from, size_t to) noexcept {
            forEach([from, to](auto p) {
                using T = typename std::decay<decltype(*p)>::type;
                // note: scalar types like int/float get initialized to zero
                for (size_t i = from; i < to; i++) {
                    new(p + i) T();
                }
                });
        }

        void destroy_each(size_t from, size_t to) noexcept {
            forEach([from, to](auto p) {
                using T = typename std::decay<decltype(*p)>::type;
                for (size_t i = from; i < to; i++) {
                    p[i].~T();
                }
                });
        }

        void move_each(void* buffer, size_t capacity) noexcept {
            auto offsets = getOffsets(capacity);
            size_t index = 0;
            if (mSize) {
                auto size = mSize; // placate a compiler warning
                forEach([buffer, &index, &offsets, size](auto p) {
                    using T = typename std::decay<decltype(*p)>::type;
                    T* SOUL_RESTRICT b = static_cast<T*>(buffer);

                    // go through each element and move them from the old array to the new
                    // then destroy them from the old array
                    T* SOUL_RESTRICT const arrayPointer =
                        reinterpret_cast<T*>(uintptr_t(b) + offsets[index]);

                    // for trivial cases, just call memcpy()
                    if (std::is_trivially_copyable<T>::value &&
                        std::is_trivially_destructible<T>::value) {
                        memcpy(arrayPointer, p, size * sizeof(T));
                    }
                    else {
                        for (size_t i = 0; i < size; i++) {
                            // we move an element by using the in-place move-constructor
                            new(arrayPointer + i) T(std::move(p[i]));
                            // and delete them by calling the destructor directly
                            p[i].~T();
                        }
                    }
                    index++;
                    });
            }

            // update the pointers (the first offset will be filled later
            for (size_t i = 1; i < kArrayCount; i++) {
                mArrayOffset[i] = (char*)buffer + offsets[i];
            }
        }

        // capacity in array elements
        size_t mCapacity = 0;
        // size in array elements
        size_t mSize = 0;
        // N pointers to each arrays
        void* mArrayOffset[kArrayCount] = { nullptr };
        soul::memory::Allocator* mAllocator = nullptr;
    };

    template<typename EnumIndex, typename... Elements>
    inline
        typename SoaPool<EnumIndex, Elements...>::StructureRef&
        SoaPool<EnumIndex, Elements...>::StructureRef::operator=(
            SoaPool::StructureRef const& rhs) {
        return operator=(Structure(rhs));
    }

    template<typename EnumIndex, typename... Elements>
    inline
        typename SoaPool<EnumIndex, Elements...>::StructureRef&
        SoaPool<EnumIndex, Elements...>::StructureRef::operator=(
            SoaPool::StructureRef&& rhs) noexcept {
        return operator=(Structure(rhs));
    }

    template<typename EnumIndex, typename... Elements>
    template<size_t... Is>
    inline
        typename SoaPool<EnumIndex, Elements...>::StructureRef&
        SoaPool<EnumIndex, Elements...>::StructureRef::assign(
            SoaPool::Structure const& rhs, std::index_sequence<Is...>) {
        // implements StructureRef& StructureRef::operator=(Structure const& rhs)
        auto SOUL_UNUSED l = { (soa->elementAt<EnumIndex(Is)>(index) = std::get<Is>(rhs.elements), 0)... };
        return *this;
    }

    template<typename EnumIndex, typename... Elements>
    template<size_t... Is>
    inline
        typename SoaPool<EnumIndex, Elements...>::StructureRef&
        SoaPool<EnumIndex, Elements...>::StructureRef::assign(
            SoaPool::Structure&& rhs, std::index_sequence<Is...>) noexcept {
        // implements StructureRef& StructureRef::operator=(Structure&& rhs) noexcept
        auto SOUL_UNUSED l = {
                (soa->elementAt<EnumIndex(Is)>(index) = std::move(std::get<Is>(rhs.elements)), 0)... };
        return *this;
    }
}