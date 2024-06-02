#pragma once

#include <limits>

#include "core/bit_vector.h"
#include "core/compiler.h"
#include "core/config.h"
#include "core/panic.h"
#include "core/type.h"
#include "core/type_traits.h"
#include "memory/allocator.h"

namespace soul
{
  template <typename T, typename RidT, memory::allocator_type AllocatorT = memory::Allocator>
  class SparsePool
  {
  private:
    static constexpr u64 SENTINEL_INDEX = std::numeric_limits<u64>::max();

  public:
    using value_type      = T;
    using pointer         = T*;
    using const_pointer   = const T*;
    using reference       = T&;
    using const_reference = const T&;

    explicit SparsePool(NotNull<AllocatorT*> allocator)
        : allocator_(allocator),
          buffer_(nullptr),
          metadatas_(nullptr),
          size_(0),
          free_list_head_(SENTINEL_INDEX),
          free_list_tail_(SENTINEL_INDEX),
          occupied_list_head_(SENTINEL_INDEX)
    {
    }

    void reserve(u64 capacity)
    {
      const b8 is_full_before = bit_vector_.size() == size_;
      T* new_buffer           = allocator_->template allocate_array<T>(capacity);
      Metadata* new_metadatas = allocator_->template allocate_array<Metadata>(capacity);
      if (buffer_ != nullptr)
      {
        relocate_objects(new_buffer, new_metadatas);
        destroy_objects();
        allocator_->deallocate_array(buffer_);
        allocator_->deallocate_array(metadatas_);
      }
      buffer_    = new_buffer;
      metadatas_ = new_metadatas;

      for (auto i = bit_vector_.size(); i < capacity - 1; i++)
      {
        metadatas_[i] = {
          .generation = 0,
          .next       = SENTINEL_INDEX,
        };
      }
      metadatas_[capacity - 1] = {
        .generation = 0,
        .next       = SENTINEL_INDEX,
      };

      if (is_full_before)
      {
        free_list_head_ = bit_vector_.size();
        free_list_tail_ = capacity - 1;
      } else
      {
        metadatas_[free_list_tail_].next = bit_vector_.size();
        free_list_tail_                  = capacity - 1;
      }
      bit_vector_.resize(capacity);
    }

    ~SparsePool()
    {
      destroy_objects();
      allocator_->deallocate_array(buffer_);
      allocator_->deallocate_array(metadatas_);
    }

    SparsePool(const SparsePool&)                        = delete;
    auto operator=(const SparsePool& rhs) -> SparsePool& = delete;
    SparsePool(SparsePool&&)                             = delete;
    auto operator=(SparsePool&& rhs) -> SparsePool&      = delete;

    template <typename... Args>
    auto create(Args&&... args) -> RidT
    {
      if (size_ == bit_vector_.size())
      {
        reserve(size_ * 2 + 8);
      }
      const auto index      = free_list_head_;
      const auto generation = metadatas_[index].generation;
      T* ptr                = buffer_ + index;
      new (ptr) T(std::forward<Args>(args)...);
      free_list_head_        = metadatas_[index].next;
      metadatas_[index].next = occupied_list_head_;
      occupied_list_head_    = index;
      bit_vector_.set(index);
      size_++;
      return RidT::Create(index, generation);
    }

    auto destroy(RidT id) -> void
    {
      buffer_[id.index()].~T();
      bit_vector_.set(id.index(), false);
      occupied_list_head_ = metadatas_[id.index()].next;
      if (size_ == bit_vector_.size())
      {
        free_list_head_ = id.index();
        free_list_tail_ = id.index();
      } else
      {
        metadatas_[free_list_tail_].next = id.index();
        free_list_tail_                  = id.index();
      }
      metadatas_[id.index()].generation++;
      metadatas_[id.index()].next = SENTINEL_INDEX;
      size_--;
    }

    [[nodiscard]]
    auto is_exist(RidT rid) const -> b8
    {
      return bit_vector_.test(rid.index()) && metadatas_[rid.index()].generation == rid.generation;
    }

    [[nodiscard]]
    auto
    operator[](RidT rid) -> reference
    {
      SOUL_ASSERT(0, is_exist(rid));
      return buffer_[rid.index()];
    }

    [[nodiscard]]
    auto
    operator[](RidT rid) const -> const_reference
    {
      SOUL_ASSERT(0, is_exist(rid));
      return buffer_[rid.index()];
    }

    [[nodiscard]]
    auto ref(RidT rid) -> reference
    {
      SOUL_ASSERT(0, is_exist(rid));
      return buffer_[rid.index()];
    }

    [[nodiscard]]
    auto cref(RidT rid) -> const_reference
    {

      SOUL_ASSERT(0, is_exist(rid));
      return buffer_[rid.index()];
    }

    [[nodiscard]]
    auto try_get(RidT rid) -> MaybeNull<T*>
    {
      if (is_exist(rid))
      {
        return buffer_ + rid.index();
      } else
      {
        return nullptr;
      }
    }

    void clear()
    {
      destroy_objects();
      for (u64 metadata_i = 0; metadata_i < bit_vector_.size(); metadata_i++)
      {
        metadatas_[metadata_i] = {
          .generation = 0,
          .next       = metadata_i + 1,
        };
      }
      metadatas_[bit_vector_.size() - 1] = {
        .generation = 0,
        .next       = SENTINEL_INDEX,
      };

      size_ = 0;
      bit_vector_.reset();
      free_list_head_     = 0;
      free_list_tail_     = bit_vector_.size() - 1;
      occupied_list_head_ = SENTINEL_INDEX;
    }

    void cleanup()
    {
      destroy_objects();
      allocator_->deallocate_array(buffer_);
      buffer_ = nullptr;
      allocator_->deallocate_array(metadatas_);
      metadatas_ = nullptr;
      size_      = 0;
      bit_vector_.cleanup();
      free_list_head_     = SENTINEL_INDEX;
      free_list_tail_     = SENTINEL_INDEX;
      occupied_list_head_ = SENTINEL_INDEX;
    }

  private:
    struct Metadata
    {
      u64 generation;
      u64 next;
    };

    NotNull<AllocatorT*> allocator_;
    BitVector<> bit_vector_;
    T* buffer_;
    Metadata* metadatas_;
    u64 size_;
    u64 free_list_head_;
    u64 free_list_tail_;
    u64 occupied_list_head_;

    void relocate_objects(T* buffer_dst, Metadata* metadatas_dst)
    {
      memcpy(metadatas_dst, metadatas_, bit_vector_.size() * sizeof(Metadata));
      if constexpr (ts_copy<T>)
      {
        memcpy(buffer_dst, buffer_, bit_vector_.size() * sizeof(T));
      } else
      {
        for (u64 i = 0; i < bit_vector_.size(); i++)
        {
          if (bit_vector_[i])
          {
            new (buffer_dst + i) T(std::move(buffer_[i].datum));
          }
        }
      }
    }

    void destroy_objects()
    {
      if constexpr (can_nontrivial_destruct_v<T>)
      {
        for (u64 i = 0; i < bit_vector_.size(); i++)
        {
          if (bit_vector_[i])
          {
            buffer_[i].~T();
          }
        }
      }
    }
  };
} // namespace soul

// namespace soul
