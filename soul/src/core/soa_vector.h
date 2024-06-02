#pragma once

#include <cstddef>
#include <utility>

#include "core/array.h"
#include "core/config.h"
#include "core/integer.h"
#include "core/panic.h"
#include "core/tuple.h"

#include "memory/allocator.h"

namespace soul
{
  template <ts_tuple TupleT, memory::allocator_type AllocatorT = memory::Allocator>
  class SoaVector;

  template <memory::allocator_type AllocatorT, typename... Ts>
  class SoaVector<Tuple<Ts...>, AllocatorT>
  {
  public:
    using structure_type        = Tuple<Ts...>;
    using structure_buffer_type = TupleX::tuple_of_pointer_t<structure_type>;

    template <usize IndexV>
    using type_at_t = structure_type::template type_at_t<IndexV>;

    static constexpr auto ELEMENT_COUNT = structure_type::ELEMENT_COUNT;
    static constexpr auto GROWTH_FACTOR = 2;

  private:
    using offset_array_type = Array<usize, ELEMENT_COUNT>;

    NotNull<AllocatorT*> allocator_;
    structure_buffer_type structure_buffers_ =
      GetStructureBuffers(nullptr, offset_array_type::Fill(0));
    usize size_     = 0;
    usize capacity_ = 0;

  public:
    explicit SoaVector(NotNull<AllocatorT*> allocator = get_default_allocator())
        : allocator_(allocator)
    {
    }

    SoaVector(const SoaVector& other) = delete;

    SoaVector(SoaVector&& other) noexcept : allocator_(other.allocator_)
    {
      swap(other);
    }

    auto operator=(const SoaVector& other) -> SoaVector& = delete;

    auto operator=(SoaVector&& other) noexcept -> SoaVector&
    {
      swap(other);
      other.cleanup();
      return *this;
    }

    ~SoaVector()
    {
      cleanup();
    }

    void swap(SoaVector& other) noexcept
    {
      using std::swap;
      swap(allocator_, other.allocator_);
      swap(structure_buffers_, other.structure_buffers_);
      swap(size_, other.size_);
      swap(capacity_, other.capacity_);
    }

    void push_back(OwnRef<Ts>... elements)
    {
      if (size_ == capacity_)
      {
        const auto new_capacity = GetNewCapacity(capacity_);
        constexpr usize alignment =
          std::max(alignof(std::max_align_t), std::ranges::max(structure_type::ELEMENT_ALIGNMENTS));
        const usize size_needed = GetNeededSize(new_capacity);

        void* new_raw_buffer      = allocator_->allocate(size_needed, alignment);
        auto const old_raw_buffer = structure_buffers_.template ref<0>();

        const auto new_offsets           = GetOffsets(new_capacity);
        const auto new_structure_buffers = GetStructureBuffers(new_raw_buffer, new_offsets);
        ConstructElements(new_structure_buffers, size_, std::move(elements)...);
        relocate_elements(new_structure_buffers);
        structure_buffers_ = new_structure_buffers;
        allocator_->deallocate(old_raw_buffer);
        capacity_ = new_capacity;
      } else
      {
        ConstructElements(structure_buffers_, size_, std::move(elements)...);
      }
      ++size_;
    }

    void pop_back()
    {
      SOUL_ASSERT(0, size_ != 0, "Cannot pop back an empty soa vector");
      size_--;
      destroy_elements(size_, 1);
    }

    void remove(usize index)
    {
      SOUL_ASSERT_UPPER_BOUND_CHECK(index, size_);
      if (size_ > 1)
      {
        structure_buffers_.for_each(
          [remove_index = index, size = size_]<usize IndexV>(auto* structure_buffer)
          {
            structure_buffer[remove_index] = std::move(structure_buffer[size - 1]);
          });
      }
      size_--;
      destroy_elements(size_, 1);
    }

    void clear()
    {
      destroy_elements(0, size_);
      size_ = 0;
    }

    void cleanup()
    {
      clear();
      allocator_->deallocate(structure_buffers_.template ref<0>());
      capacity_          = 0;
      structure_buffers_ = GetStructureBuffers(nullptr, offset_array_type::Fill(0));
    }

    template <usize StructureIndexV>
    [[nodiscard]]
    auto span() -> Span<type_at_t<StructureIndexV>*>
    {
      return {structure_buffers_.template ref<StructureIndexV>(), size_};
    }

    template <usize StructureIndexV>
    [[nodiscard]]
    auto span() const -> Span<const type_at_t<StructureIndexV>*>
    {
      return {structure_buffers_.template ref<StructureIndexV>(), size_};
    }

    template <usize StructureIndexV>
    [[nodiscard]]
    auto cspan() const -> Span<const type_at_t<StructureIndexV>*>
    {
      return {structure_buffers_.template ref<StructureIndexV>(), size_};
    }

    [[nodiscard]]
    auto size() const -> usize
    {
      return size_;
    }

    [[nodiscard]]
    auto empty() const -> b8
    {
      return size_ == 0;
    }

    template <usize StrucureIndexV>
    [[nodiscard]]
    constexpr auto ref(usize element_index) const -> const type_at_t<StrucureIndexV>&
    {
      SOUL_ASSERT_UPPER_BOUND_CHECK(element_index, size_);
      return structure_buffers_.template ref<StrucureIndexV>()[element_index];
    }

    template <usize StructureIndexV>
    [[nodiscard]]
    constexpr auto ref(usize element_index) -> type_at_t<StructureIndexV>&
    {
      SOUL_ASSERT_UPPER_BOUND_CHECK(element_index, size_);
      return structure_buffers_.template ref<StructureIndexV>()[element_index];
    }

    template <usize StrucureIndexV>
    [[nodiscard]]
    constexpr auto back_cref() const -> const type_at_t<StrucureIndexV>&
    {
      SOUL_ASSERT(0, !empty(), "");
      const auto element_index = size() - 1;
      return structure_buffers_.template ref<StrucureIndexV>()[element_index];
    }

    template <usize StructureIndexV>
    [[nodiscard]]
    constexpr auto back_ref() -> type_at_t<StructureIndexV>&
    {
      SOUL_ASSERT(0, !empty(), "");
      const auto element_index = size() - 1;
      return structure_buffers_.template ref<StructureIndexV>()[element_index];
    }

    template <typename Fn>
    void for_each(usize element_index, Fn fn)
    {
      structure_buffers_.for_each(
        [&fn, element_index]<usize IndexV>(auto* structure_buffer)
        {
          fn.template operator()<IndexV>(structure_buffer[element_index]);
        });
    };

    template <typename Fn>
    void for_each(usize element_index, Fn fn) const
    {
      structure_buffers_.for_each(
        [&fn, element_index]<usize IndexV>(const auto* structure_buffer)
        {
          fn.template operator()<IndexV>(structure_buffer[element_index]);
        });
    };

  private:
    static inline auto GetOffset(usize index, usize capacity) -> usize
    {
      auto offsets = GetOffsets(capacity);
      return offsets[index];
    }

    static inline auto GetOffsets(size_t capacity) -> Array<usize, ELEMENT_COUNT>
    {
      const auto sizes = Array<usize, ELEMENT_COUNT>::TransformIndex(
        [capacity](usize idx) -> usize
        {
          const auto element_size = structure_type::ELEMENT_SIZES[idx];
          return element_size * capacity;
        });

      // we align each array to at least the same alignment guaranteed by malloc
      constexpr auto alignments = Array<usize, ELEMENT_COUNT>::TransformIndex(
        [](usize idx) -> usize
        {
          const auto element_alignment = structure_type::ELEMENT_ALIGNMENTS[idx];
          return std::max(alignof(std::max_align_t), element_alignment);
        });

      Array<usize, ELEMENT_COUNT> offsets;
      offsets[0] = 0;
      SOUL_UNROLL
      for (usize i = 1; i < ELEMENT_COUNT; i++)
      {
        usize unalignment = (offsets[i - 1] + sizes[i - 1]) % alignments[i];
        usize alignment   = unalignment != 0u ? (alignments[i] - unalignment) : 0;
        offsets[i]        = offsets[i - 1] + (sizes[i - 1] + alignment);
        SOUL_ASSERT(0, offsets[i] % alignments[i] == 0);
      }
      return offsets;
    }

    [[nodiscard]]
    static auto GetNeededSize(usize capacity) -> size_t
    {
      return GetOffset(ELEMENT_COUNT - 1, capacity) +
             structure_type::ELEMENT_SIZES[ELEMENT_COUNT - 1] * capacity;
    }

    [[nodiscard]]
    static auto GetNewCapacity(usize old_capacity) -> usize
    {
      return old_capacity * GROWTH_FACTOR + 8;
    }

    [[nodiscard]]
    static auto GetStructureBuffers(void* raw_buffer, const offset_array_type& offsets)
      -> structure_buffer_type
    {
      const auto create_tuple =
        [raw_buffer, &offsets]<usize... idx>(std::index_sequence<idx...>) -> structure_buffer_type
      {
        return structure_buffer_type{
          (reinterpret_cast<structure_buffer_type::template type_at_t<idx>>(
            uptr(raw_buffer) + offsets[idx]))...};
      };
      return create_tuple(std::make_index_sequence<ELEMENT_COUNT>());
    }

    template <usize StructureIndexV = 0, typename ConstructT, typename... ConstructTs>
    void ConstructElements(
      const structure_buffer_type& structure_buffers,
      usize element_index,
      OwnRef<ConstructT> element,
      OwnRef<ConstructTs>... elements)
    {
      auto* structure_buffer = structure_buffers.template ref<StructureIndexV>();
      construct_at(structure_buffer + element_index, std::move(element));
      if constexpr (sizeof...(elements) != 0)
      {
        ConstructElements<StructureIndexV + 1>(
          structure_buffers, element_index, std::move(elements)...);
      }
    }

    void destroy_elements(usize start_idx, usize count) noexcept
    {
      structure_buffers_.for_each(
        [start_idx, count]<usize IndexV>(auto* buffer)
        {
          destroy_n(buffer + start_idx, count);
        });
    }

    void relocate_elements(const structure_buffer_type& structure_buffers) noexcept
    {
      structure_buffers_.for_each(
        [&structure_buffers, size = size_]<usize IndexV>(auto* src_buffer)
        {
          using element_type       = type_at_t<IndexV>;
          element_type* dst_buffer = structure_buffers.template ref<IndexV>();
          uninitialized_relocate_n(src_buffer, size, dst_buffer);
        });
    }
  };

} // namespace soul
