#pragma once

#include <limits>
#include <mutex>

#include "core/bit_vector.h"
#include "core/boolean.h"
#include "core/compiler.h"
#include "core/config.h"
#include "core/deque.h"
#include "core/mutex.h"
#include "core/panic.h"
#include "core/soa_vector.h"
#include "core/tuple.h"
#include "core/type.h"
#include "core/type_traits.h"

#include "memory/allocator.h"

namespace soul
{
  template <
    typename T,
    typename RidT,
    is_lockable_v LockT               = Mutex,
    u64 ElementCountPerChunkV         = 4096 / sizeof(T),
    memory::allocator_type AllocatorT = memory::Allocator>
  class ChunkedSparsePool
  {
  private:
    static constexpr u64 SENTINEL_INDEX         = std::numeric_limits<u64>::max();
    static constexpr u64 OBJECT_COUNT_PER_CHUNK = ElementCountPerChunkV;

  public:
    using value_type      = T;
    using rid_type        = RidT;
    using pointer         = T*;
    using const_pointer   = const T*;
    using reference       = T&;
    using const_reference = const T&;

    explicit ChunkedSparsePool(NotNull<AllocatorT*> allocator = get_default_allocator())
        : allocator_(allocator),
          free_list_head_(SENTINEL_INDEX),
          free_list_tail_(SENTINEL_INDEX),
          occupied_list_head_(SENTINEL_INDEX)
    {
    }

    ~ChunkedSparsePool()
    {
      destroy_objects();
      for (u64 chunk_i = 0; chunk_i < chunks_.size(); chunk_i++)
      {
        allocator_->deallocate_array(chunks_.template ref<0>(chunk_i), OBJECT_COUNT_PER_CHUNK);
        allocator_->deallocate_array(chunks_.template ref<1>(chunk_i), OBJECT_COUNT_PER_CHUNK);
      }
    }

    ChunkedSparsePool(const ChunkedSparsePool&)                        = delete;
    auto operator=(const ChunkedSparsePool& rhs) -> ChunkedSparsePool& = delete;
    ChunkedSparsePool(ChunkedSparsePool&&)                             = delete;
    auto operator=(ChunkedSparsePool&& rhs) -> ChunkedSparsePool&      = delete;

    template <typename... Args>
    auto create(Args&&... args) -> RidT
    {
      std::lock_guard guard(lock_);
      if (size_ == capacity())
      {
        T* new_buffer_chunk = allocator_->template allocate_array<T>(OBJECT_COUNT_PER_CHUNK);

        Metadata* new_metadata_chunk =
          allocator_->template allocate_array<Metadata>(OBJECT_COUNT_PER_CHUNK);
        for (u64 metadata_i = 0; metadata_i < OBJECT_COUNT_PER_CHUNK - 1; metadata_i++)
        {
          new_metadata_chunk[metadata_i] = Metadata(capacity() + metadata_i + 1);
        }
        new_metadata_chunk[OBJECT_COUNT_PER_CHUNK - 1] = Metadata(SENTINEL_INDEX);

        free_list_head_ = capacity();
        free_list_tail_ = free_list_head_ + OBJECT_COUNT_PER_CHUNK - 1;

        chunks_.push_back(new_buffer_chunk, new_metadata_chunk);
      }
      const auto index      = free_list_head_;
      auto& metadata        = metadata_ref(index);
      const auto generation = metadata.generation();
      const auto ptr =
        chunks_.template ref<0>(index / OBJECT_COUNT_PER_CHUNK) + (index % OBJECT_COUNT_PER_CHUNK);
      new (ptr) T(std::forward<Args>(args)...);
      free_list_head_     = metadata.next;
      metadata.next       = occupied_list_head_;
      occupied_list_head_ = index;
      metadata.set_occupied();
      size_++;

      return RidT::Create(index, generation);
    }

    void destroy(RidT id)
    {
      SOUL_ASSERT(0, is_alive(id));
      ref(id).~T();

      std::lock_guard guard(lock_);
      if (size_ == capacity())
      {
        free_list_head_ = id.index();
        free_list_tail_ = id.index();
      } else
      {
        metadata_ref(free_list_tail_).next = id.index();
        free_list_tail_                    = id.index();
      }
      auto& metadata = metadata_ref(id.index());
      metadata.set_unoccupied();
      occupied_list_head_ = metadata.next;
      metadata.inc_generation();
      metadata.next = SENTINEL_INDEX;
      size_--;
    }

    [[nodiscard]]
    auto size() const -> u64
    {
      return size_;
    }

    [[nodiscard]]
    auto is_empty() const -> b8
    {
      return size_ == 0;
    }

    [[nodiscard]]
    auto is_alive(RidT rid) const -> b8
    {
      std::lock_guard guard(lock_);
      if (rid.index() >= capacity())
      {
        return false;
      }
      const auto& metadata = metadata_cref(rid.index());
      return metadata.is_occupied() && metadata.generation() == rid.generation();
    }

    [[nodiscard]]
    auto ref(RidT rid) -> reference
    {
      SOUL_ASSERT(0, is_alive(rid), "Reference a Rid that is not alive");
      const auto index = rid.index();
      std::lock_guard guard(lock_);
      return chunks_.template ref<0>(
        index / OBJECT_COUNT_PER_CHUNK)[index % OBJECT_COUNT_PER_CHUNK];
    }

    [[nodiscard]]
    auto cref(RidT rid) const -> const_reference
    {
      SOUL_ASSERT(0, is_alive(rid), "Reference a Rid that is not alive");
      const auto index = rid.index();
      std::lock_guard guard(lock_);
      return chunks_.template ref<0>(
        index / OBJECT_COUNT_PER_CHUNK)[index % OBJECT_COUNT_PER_CHUNK];
    }

    [[nodiscard]]
    auto
    operator[](RidT rid) -> reference
    {
      return ref(rid);
    }

    [[nodiscard]]
    auto
    operator[](RidT rid) const -> const_reference
    {
      return cref(rid);
    }

    [[nodiscard]]
    auto try_ptr(RidT rid) -> MaybeNull<T*>
    {
      if (!is_alive())
      {
        return nullptr;
      }
      const auto index = rid.index();
      return &chunks_.template ref<0>(
        index / OBJECT_COUNT_PER_CHUNK)[index % OBJECT_COUNT_PER_CHUNK];
    }

    [[nodiscard]]
    auto try_cptr(RidT rid) const -> MaybeNull<const T*>
    {
      if (!is_alive())
      {
        return nullptr;
      }
      const auto index = rid.index();
      return &chunks_.template ref<0>(
        index / OBJECT_COUNT_PER_CHUNK)[index % OBJECT_COUNT_PER_CHUNK];
    }

    [[nodiscard]]
    auto capacity() const -> u64
    {
      return chunks_.size() * OBJECT_COUNT_PER_CHUNK;
    }

    void clear()
    {
      if (size_ == 0)
      {
        return;
      }

      destroy_objects();
      u64 slot_i = 0;
      for (u64 chunk_i = 0; chunk_i < chunks_.size(); chunk_i++)
      {
        for (u64 object_i = 0; object_i < OBJECT_COUNT_PER_CHUNK; object_i++)
        {
          auto& metadata = chunks_.template ref<1>(chunk_i)[object_i];
          metadata.set_unoccupied();
          metadata.next = slot_i + 1;
          slot_i++;
        }
      }
      metadata_back_ref().next = SENTINEL_INDEX;
      free_list_head_          = 0;
      free_list_tail_          = size_ - 1;
      occupied_list_head_      = SENTINEL_INDEX;
      size_                    = 0;
    }

    void cleanup()
    {
      destroy_objects();
      for (u64 chunk_i = 0; chunk_i < chunks_.size(); chunk_i++)
      {
        allocator_->deallocate_array(chunks_.template ref<0>(chunk_i), OBJECT_COUNT_PER_CHUNK);
        allocator_->deallocate_array(chunks_.template ref<1>(chunk_i), OBJECT_COUNT_PER_CHUNK);
      }
      chunks_.cleanup();
      free_list_head_     = SENTINEL_INDEX;
      free_list_tail_     = SENTINEL_INDEX;
      occupied_list_head_ = SENTINEL_INDEX;
      size_               = 0;
    }

  private:
    struct Metadata
    {
      static constexpr u64 OCCUPIED_MASK   = (u64(1) << 63);
      static constexpr u64 GENERATION_MASK = ~u64(0) - OCCUPIED_MASK;
      u64 next;

      explicit Metadata(u64 next_in) : next(next_in), occupied_and_generation_(0) {}

      [[nodiscard]]
      auto generation() const -> u64
      {
        return occupied_and_generation_ & GENERATION_MASK;
      }

      [[nodiscard]]
      auto is_occupied() const -> b8
      {
        return (occupied_and_generation_ & OCCUPIED_MASK) != 0u;
      }

      void set_occupied()
      {
        occupied_and_generation_ |= OCCUPIED_MASK;
      }

      void set_unoccupied()
      {
        occupied_and_generation_ &= GENERATION_MASK;
      }

      void inc_generation()
      {
        occupied_and_generation_++;
      }

    private:
      u64 occupied_and_generation_;
    };

    auto metadata_ref(u64 index) -> Metadata&
    {
      return chunks_.template ref<1>(
        index / OBJECT_COUNT_PER_CHUNK)[index % OBJECT_COUNT_PER_CHUNK];
    }

    auto metadata_cref(u64 index) const -> const Metadata&
    {
      return chunks_.template ref<1>(
        index / OBJECT_COUNT_PER_CHUNK)[index % OBJECT_COUNT_PER_CHUNK];
    }

    auto metadata_back_ref() -> Metadata&
    {
      return chunks_.template back_ref<1>()[OBJECT_COUNT_PER_CHUNK - 1];
    }

    void destroy_objects()
    {
      if constexpr (can_nontrivial_destruct_v<T>)
      {
        for (u64 chunk_i = 0; chunk_i < chunks_.size(); chunk_i++)
        {
          for (u64 object_i = 0; object_i < OBJECT_COUNT_PER_CHUNK; object_i++)
          {
            if (chunks_.template ref<1>(chunk_i)[object_i].is_occupied())
            {
              T* obj_ptr = chunks_.template ref<0>(chunk_i) + object_i;
              destroy_at(obj_ptr);
            }
          }
        }
      }
    }

    mutable LockT lock_;
    NotNull<AllocatorT*> allocator_;
    SoaVector<Tuple<T*, Metadata*>> chunks_;
    u64 size_{};
    u64 free_list_head_;
    u64 free_list_tail_;
    u64 occupied_list_head_;
  };
} // namespace soul
