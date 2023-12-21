#pragma once

#include "core/array.h"
#include "core/builtins.h"
#include "core/compiler.h"
#include "core/config.h"
#include "core/hash.h"
#include "core/log.h"
#include "core/meta.h"
#include "core/objops.h"
#include "core/panic_lite.h"
#include "core/span.h"
#include "math/math.h"
#include "memory/allocator.h"

namespace soul
{
  namespace impl
  {
    struct RobinTableMetadata {
      using storage_type = u8;
      storage_type bits;
      static constexpr u8 TOTAL_BIT_COUNT = sizeof(storage_type) * 8;
      static constexpr u8 PSL_BIT_COUNT = 5;
      static constexpr storage_type PSL_SHIFT_COUNT = TOTAL_BIT_COUNT - PSL_BIT_COUNT;
      static constexpr storage_type PSL_INC = 1 << PSL_SHIFT_COUNT;
      static constexpr storage_type PSL_MAX = (1 << PSL_BIT_COUNT) - 2;

      static constexpr storage_type HOISTED_HASH_BIT_COUNT = TOTAL_BIT_COUNT - PSL_BIT_COUNT;
      static constexpr storage_type HOISTED_HASH_MASK = PSL_INC - 1;
      static constexpr storage_type PSL_MASK = ~HOISTED_HASH_MASK;
      static constexpr storage_type SENTINEL_BITS = ~u8(0);

      [[nodiscard]]
      static constexpr auto HoistHash(u64 hash_code) -> storage_type
      {
        const auto hoist_hash_code = hash_code & HOISTED_HASH_MASK;
        return hoist_hash_code;
      }

      [[nodiscard]]
      static constexpr auto Sentinel() -> RobinTableMetadata
      {
        return {.bits = SENTINEL_BITS};
      }

      [[nodiscard]]
      static constexpr auto Empty() -> RobinTableMetadata
      {
        return {.bits = 0};
      }

      [[nodiscard]]
      static constexpr auto FromHash(u64 hash_code) -> RobinTableMetadata
      {
        return RobinTableMetadata{.bits = storage_type(PSL_INC | HoistHash(hash_code))};
      }

      [[nodiscard]]
      auto is_psl_overflow() const -> b8
      {
        return (bits & PSL_MASK) == PSL_MASK;
      }

      [[nodiscard]]
      auto is_empty() const -> b8
      {
        return bits == 0;
      }

      [[nodiscard]]
      auto is_sentinel() const -> b8
      {
        return bits == SENTINEL_BITS;
      }

      [[nodiscard]]
      auto is_psl_greater_than_one() const -> b8
      {
        return bits >= (PSL_INC << u8(1));
      }
      void increment_psl() { bits += PSL_INC; }

      void decrement_psl() { bits -= PSL_INC; }

      [[nodiscard]]
      auto get_hoisted_hash() const -> storage_type
      {
        return bits & HOISTED_HASH_MASK;
      }

      [[nodiscard]]
      auto get_psl() const -> storage_type
      {
        return bits >> PSL_SHIFT_COUNT;
      }

      friend void swap(RobinTableMetadata& lhs, RobinTableMetadata& rhs) noexcept
      {
        using std::swap;
        swap(lhs.bits, rhs.bits);
      }

      static void InitMetadatas(Span<RobinTableMetadata*, usize> metadatas)
      {
        const auto size = metadatas.size();
        memset(metadatas.data(), 0, sizeof(RobinTableMetadata) * (size - 1));
        metadatas[size - 1] = RobinTableMetadata::Sentinel();
      }

      auto operator<=>(const RobinTableMetadata& other) const = default;
    };

    // use for default no allocation state hash map
    static auto ROBIN_TABLE_METADATA_DUMMY_SENTINEL = RobinTableMetadata::Sentinel(); // NOLINT

  } // namespace impl

  struct RobinTableConfig {
    f32 load_factor = 0.5f;
  };

  template <
    typename KeyT,
    typename EntryT,
    typename GetKeyFn,
    typename HashFn = soul::HashOp<KeyT>,
    RobinTableConfig ConfigV = RobinTableConfig(),
    memory::allocator_type AllocatorT = memory::Allocator>
  class RobinTable
  {
    using Metadata = impl::RobinTableMetadata;

  public:
    template <b8 IsConstV>
    class Iterator
    {
    private:
      Metadata* metadata_iter_ = nullptr;

      using IterEntryT = std::conditional_t<IsConstV, EntryT, const EntryT>;
      IterEntryT* entry_iter_ = nullptr;

      explicit Iterator(Metadata* metadatas, IterEntryT* entries)
          : metadata_iter_(metadatas), entry_iter_(entries)
      {
        while (metadata_iter_->is_empty()) {
          metadata_iter_++;
          entry_iter_++;
        }
        if (metadata_iter_->is_sentinel()) {
          *this = Iterator();
        }
      }

      friend class RobinTable;

    public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = IterEntryT;
      using reference = IterEntryT&;
      using difference_type = std::ptrdiff_t;
      using pointer = IterEntryT*;

      explicit Iterator() = default;

      template <b8 IsOtherConstV>
        requires(!IsConstV && !IsOtherConstV)
      explicit Iterator(const Iterator<IsOtherConstV>& other) noexcept
          : metadata_iter_(other.metadata_iter_), entry_iter_(other.entry_iter_)
      {
      }

      auto operator++() noexcept -> Iterator&
      {
        do {
          ++metadata_iter_;
          ++entry_iter_;
        } while (metadata_iter_->is_empty());
        if (metadata_iter_->is_sentinel()) {
          *this = Iterator();
        }
        return *this;
      }

      auto operator++(int) noexcept -> Iterator
      {
        Iterator iter_copy(metadata_iter_, entry_iter_);
        ++*this;
        return iter_copy;
      }

      auto operator*() const noexcept -> reference { return *entry_iter_; }

      auto operator->() const noexcept -> pointer { return entry_iter_; }

      template <b8 IsOtherConstV>
      auto operator==(Iterator<IsOtherConstV> const& other) const noexcept -> b8
      {
        return entry_iter_ == other.entry_iter_;
      }
    };

    using value_type = EntryT;
    using key_type = KeyT;
    using reference = EntryT&;
    using const_reference = const EntryT&;
    using pointer = EntryT*;
    using const_pointer = const EntryT*;
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;
    using sentinel = iterator;
    using const_sentinel = const_iterator;

  private:
    static constexpr u8 HASH_CODE_BIT_COUNT_ = 64;
    static constexpr u8 INITIAL_SHIFTS_ = HASH_CODE_BIT_COUNT_ - 3;

    AllocatorT* allocator_ = nullptr;
    u8 shifts_ = INITIAL_SHIFTS_;
    u64 slot_count_ = 0;
    u64 capacity_ = 0;
    u64 size_ = 0;

    Metadata* metadatas_ = &impl::ROBIN_TABLE_METADATA_DUMMY_SENTINEL;
    EntryT* entries_ = nullptr;

    SOUL_NO_UNIQUE_ADDRESS HashFn hash_fn_;
    SOUL_NO_UNIQUE_ADDRESS GetKeyFn get_key_fn_;

    RobinTable(const RobinTable& other)
        : allocator_(other.allocator_),
          shifts_(other.shifts_),
          slot_count_(other.slot_count_),
          capacity_(other.capacity_),
          size_(other.size_),
          metadatas_(
            other.slot_count_ == 0
              ? &impl::ROBIN_TABLE_METADATA_DUMMY_SENTINEL
              : allocator_->template allocate_array<Metadata>(other.slot_count_ + 1)),
          entries_(allocator_->template allocate_array<EntryT>(other.slot_count_))
    {
      memcpy(metadatas_, other.metadatas_, sizeof(Metadata) * (slot_count_ + 1));
      duplicate_entries(other);
    }

    auto operator=(const RobinTable& other) -> RobinTable&
    {
      RobinTable tmp(other);
      swap(tmp, *this);
      return *this;
    }

    struct Construct {
      struct WithCapacity {
      };
      struct From {
      };
      static constexpr auto with_capacity = WithCapacity{};
      static constexpr auto from = From{};
    };

    RobinTable(Construct::WithCapacity /* tag */, usize min_capacity, AllocatorT& allocator)
        : allocator_(&allocator), shifts_(ComputeShiftsForBucketCount(min_capacity))
    {
      allocate_slots_from_shift();
    }

    template <std::ranges::input_range RangeT>
    RobinTable(Construct::From /* tag */, RangeT&& range, AllocatorT& allocator)
        : allocator_(&allocator)
    {
      if constexpr (std::ranges::sized_range<RangeT> || std::ranges::forward_range<RangeT>) {
        const auto size = usize(std::ranges::distance(range));
        do_reserve(size);
      }
      const auto last = std::ranges::end(range);
      for (auto it = std::ranges::begin(range); it != last; it++) {
        insert(*it);
      }
    }

    [[nodiscard]]
    auto slot_count() const -> usize
    {
      return slot_count_;
    };

    [[nodiscard]]
    auto metadata_count() const -> usize
    {
      return slot_count_ + 1;
    }

    [[nodiscard]]
    constexpr auto home_index_from_hash(u64 hash_code) const -> u64
    {
      return hash_code >> shifts_;
    };

    [[nodiscard]]
    auto next_bucket_index(usize bucket_index)
    {
      return (bucket_index + 1);
    }

    [[nodiscard]]
    static constexpr auto ComputeBucketCount(u8 shifts) -> usize
    {
      return 1u << (HASH_CODE_BIT_COUNT_ - shifts);
    }

    [[nodiscard]]
    static constexpr auto ComputeShiftsForBucketCount(usize min_bucket) -> u8
    {
      auto shifts = INITIAL_SHIFTS_;
      while (shifts > 0 &&
             static_cast<usize>(
               static_cast<f32>(ComputeBucketCount(shifts)) * ConfigV.load_factor) < min_bucket) {
        --shifts;
      }
      return shifts;
    }

    void allocate_slots_from_shift()
    {
      const auto bucket_count = ComputeBucketCount(shifts_);
      slot_count_ = bucket_count + Metadata::PSL_MAX + 1;
      capacity_ = ConfigV.load_factor * bucket_count;

      metadatas_ = allocator_->template allocate_array<Metadata>(slot_count_ + 1);
      Metadata::InitMetadatas({metadatas_, slot_count_ + 1});
      entries_ = allocator_->template allocate_array<EntryT>(slot_count_);
    }

    void do_reserve(usize min_capacity)
    {
      const auto old_capacity = capacity_;
      const auto old_slot_count = slot_count_;
      Metadata* old_metadatas = metadatas_;
      EntryT* old_entries = entries_;
      shifts_ = ComputeShiftsForBucketCount(min_capacity);
      allocate_slots_from_shift();
      if (old_slot_count != 0) {
        size_ = 0;
        for (usize bucket_index = 0; bucket_index < old_slot_count; bucket_index++) {
          if (!old_metadatas[bucket_index].is_empty()) {
            do_insert(std::move(old_entries[bucket_index]));
            destroy_at(&old_entries[bucket_index]);
          }
        }
        allocator_->deallocate_array(old_metadatas, old_slot_count + 1);
        allocator_->deallocate_array(old_entries, old_slot_count);
      }
    }

    void do_insert(OwnRef<EntryT, true> entry)
    {
      const auto& key = get_key_fn_(entry.const_ref());
      const auto hash_code = hash_fn_(key);
      const auto home_index = home_index_from_hash(hash_code);
      const auto expected_max_psl = math::floor_log2(size_ + 1) + 2;
      auto bucket_index = home_index;
      auto metadata = Metadata::FromHash(hash_code);
      while (!metadatas_[bucket_index].is_empty()) {
        auto& current_entry = entries_[bucket_index];
        if (metadata == metadatas_[bucket_index] && key == get_key_fn_(current_entry)) {
          current_entry = entry.forward_ref();
          return;
        } else if (metadatas_[bucket_index] < metadata) {
          entry.swap_at(&current_entry);
          swap(metadata, metadatas_[bucket_index]);
        }
        metadata.increment_psl();
        bucket_index++;
        if (metadata.is_psl_overflow()) {
          SOUL_PANIC_LITE("RobinTable: PSL overflow");
        }
        SOUL_ASSERT(
          0,
          metadata.get_psl() <= expected_max_psl,
          "Robin table psl({}) reach higher than max expected psl({})",
          metadata.get_psl(),
          expected_max_psl);
      }
      entry.store_at(&entries_[bucket_index]);
      metadatas_[bucket_index] = metadata;
      size_++;
    }

    template <typename QueryT>
      requires(BorrowTrait<KeyT, QueryT>::available)
    auto do_find_index(QueryT key) const -> usize
    {
      if (SOUL_UNLIKELY(empty())) {
        return slot_count();
      }
      const auto hash_code = hash_fn_(key);
      const auto home_index = home_index_from_hash(hash_code);
      auto slot_index = home_index;
      auto metadata = Metadata::FromHash(hash_code);
      while (!metadatas_[slot_index].is_empty()) {
        if (metadata == metadatas_[slot_index]) {
          const auto& stored_key = get_key_fn_(entries_[slot_index]);
          if (key == borrow<QueryT>(stored_key)) {
            return slot_index;
          }
        }
        metadata.increment_psl();
        slot_index++;
      }
      return slot_count();
    }

    auto do_find_index(const KeyT& key) const -> usize
    {
      if (SOUL_UNLIKELY(empty())) {
        return slot_count();
      }
      const auto hash_code = hash_fn_(key);
      const auto home_index = home_index_from_hash(hash_code);
      auto slot_index = home_index;
      auto metadata = Metadata::FromHash(hash_code);
      while (!metadatas_[slot_index].is_empty()) {
        if (metadata == metadatas_[slot_index]) {
          if (key == get_key_fn_(entries_[slot_index])) {
            return slot_index;
          }
        }
        metadata.increment_psl();
        slot_index++;
      }
      return slot_count();
    }

    void do_remove_index(usize prev_bucket_index)
    {
      if (prev_bucket_index == slot_count()) {
        return;
      }
      auto bucket_index = next_bucket_index(prev_bucket_index);
      while (metadatas_[bucket_index].is_psl_greater_than_one() && bucket_index < slot_count()) {
        metadatas_[prev_bucket_index] = metadatas_[bucket_index];
        metadatas_[prev_bucket_index].decrement_psl();
        entries_[prev_bucket_index] = std::move(entries_[bucket_index]);
        prev_bucket_index = bucket_index;
        bucket_index = next_bucket_index(bucket_index);
      }
      metadatas_[prev_bucket_index] = Metadata::Empty();
      destroy_at(&entries_[prev_bucket_index]);
      size_--;
    }

    void duplicate_entries(const RobinTable& other)
    {
      if constexpr (ts_copy<EntryT>) {
        memcpy(entries_, other.entries_, sizeof(EntryT) * other.capacity_);
      } else if constexpr (ts_clone<EntryT>) {
        for (usize slot_index = 0; slot_index < other.slot_count(); ++slot_index) {
          if (metadatas_[slot_index].is_empty()) {
            continue;
          }
          clone_at(entries_ + slot_index, other.entries_[slot_index]);
        }
      } else {
        for (usize slot_index = 0; slot_index < other.slot_count(); ++slot_index) {
          if (metadatas_[slot_index].is_empty()) {
            continue;
          }
          construct_at(entries_ + 1, other.entries_[slot_index]);
        }
      }
    }

    void destruct_entries()
    {
      if constexpr (can_nontrivial_destruct_v<EntryT>) {
        for (usize slot_index = 0; slot_index < slot_count(); ++slot_index) {
          if (!metadatas_[slot_index].is_empty()) {
            destroy_at(&entries_[slot_index]);
          }
        }
      }
    }

  public:
    explicit RobinTable(AllocatorT& allocator = *get_default_allocator()) : allocator_(&allocator)
    {
    }

    RobinTable(RobinTable&& other) noexcept
        : allocator_(std::exchange(other.allocator_, nullptr)),
          shifts_(std::exchange(other.shifts_, INITIAL_SHIFTS_)),
          slot_count_(std::exchange(other.slot_count_, 0)),
          capacity_(std::exchange(other.capacity_, 0)),
          size_(std::exchange(other.size_, 0)),
          metadatas_(std::exchange(other.metadatas_, &impl::ROBIN_TABLE_METADATA_DUMMY_SENTINEL)),
          entries_(std::exchange(other.entries_, nullptr))
    {
    }

    auto operator=(RobinTable&& other) noexcept -> RobinTable&
    {
      auto tmp = RobinTable(std::move(other));
      swap(tmp, *this);
      return *this;
    }

    ~RobinTable()
    {
      if (slot_count_ != 0) {
        destruct_entries();
        allocator_->deallocate_array(metadatas_, slot_count_ + 1);
        allocator_->deallocate_array(entries_, slot_count_);
      }
    }

    static auto WithCapacity(usize min_capacity, AllocatorT& allocator = *get_default_allocator())
      -> RobinTable
    {
      return RobinTable(Construct::with_capacity, min_capacity, allocator);
    }

    template <std::ranges::input_range RangeT>
    static auto From(RangeT&& range, AllocatorT& allocator = *get_default_allocator()) -> RobinTable
    {
      return RobinTable(Construct::from, std::forward<RangeT>(range), allocator);
    }

    [[nodiscard]]
    auto clone() const -> RobinTable
    {
      return RobinTable(*this);
    }

    void clone_from(const RobinTable& other) { *this = other; }

    friend void swap(RobinTable& lhs, RobinTable& rhs)
    {
      using std::swap;
      swap(lhs.allocator_, rhs.allocator_);
      swap(lhs.shifts_, rhs.shifts_);
      swap(lhs.slot_count_, rhs.slot_count_);
      swap(lhs.capacity_, rhs.capacity_);
      swap(lhs.size_, rhs.size_);
      swap(lhs.metadatas_, rhs.metadatas_);
      swap(lhs.entries_, rhs.entries_);
    }

    void clear()
    {
      destruct_entries();
      Metadata::InitMetadatas({metadatas_, slot_count_ + 1});
      size_ = 0;
    }

    void cleanup()
    {
      if (slot_count_ != 0) {
        destruct_entries();
        allocator_->deallocate_array(metadatas_, slot_count_ + 1);
        allocator_->deallocate_array(entries_, slot_count_);

        shifts_ = INITIAL_SHIFTS_;
        slot_count_ = 0;
        capacity_ = 0;
        size_ = 0;
        metadatas_ = &impl::ROBIN_TABLE_METADATA_DUMMY_SENTINEL;
        entries_ = nullptr;
      }
    }

    template <typename QueryT>
      requires(BorrowTrait<KeyT, QueryT>::available)
    [[nodiscard]]
    auto contains(QueryT key) const -> b8
    {
      return do_find_index(key) != slot_count();
    }

    [[nodiscard]]
    auto contains(const KeyT& key) const -> b8
    {
      return do_find_index(key) != slot_count();
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

    [[nodiscard]]
    auto capacity() const -> usize
    {
      return capacity_;
    }

    [[nodiscard]]
    auto begin() -> iterator
    {
      return iterator(metadatas_, entries_);
    }

    [[nodiscard]]
    auto begin() const -> const_iterator
    {
      return const_iterator(metadatas_, entries_);
    }

    [[nodiscard]]
    auto cbegin() const -> const_iterator
    {
      return begin();
    }

    [[nodiscard]]
    auto end() -> sentinel
    {
      return sentinel();
    }

    [[nodiscard]]
    auto end() const -> const_sentinel
    {
      return const_sentinel();
    }

    [[nodiscard]]
    auto cend() const -> const_sentinel
    {
      return const_sentinel();
    }

    void reserve(usize capacity)
    {
      if (capacity > capacity_) {
        do_reserve(capacity);
      }
    }

    void insert(OwnRef<EntryT, true> entry)
    {
      if (size_ + 1 > capacity_) {
        const auto old_capacity = capacity_;
        const auto old_slot_count = slot_count_;
        Metadata* old_metadatas = metadatas_;
        EntryT* old_entries = entries_;

        shifts_--;
        allocate_slots_from_shift();

        if (old_slot_count == 0) {
          do_insert(entry.forward());
        } else {
          size_ = 0;
          for (usize bucket_index = 0; bucket_index < old_slot_count; bucket_index++) {
            if (!old_metadatas[bucket_index].is_empty()) {
              do_insert(std::move(old_entries[bucket_index]));
              destroy_at(&old_entries[bucket_index]);
            }
          }
          do_insert(entry.forward());
          allocator_->deallocate_array(old_metadatas, slot_count_ + 1);
          allocator_->deallocate_array(old_entries, old_slot_count);
        }
      } else {
        do_insert(entry.forward());
      }
    }

    template <typename QueryT>
      requires(BorrowTrait<KeyT, QueryT>::available)
    auto find(QueryT key) -> iterator
    {
      if (slot_count_ == 0) {
        return end();
      }
      const auto index = do_find_index(key);
      return iterator(metadatas_ + index, entries_ + index);
    }

    template <typename QueryT>
      requires(BorrowTrait<KeyT, QueryT>::available)
    auto find(QueryT key) const -> const_iterator
    {
      if (slot_count_ == 0) {
        return end();
      }
      const auto index = do_find_index(key);
      return const_iterator(metadatas_ + index, entries_ + index);
    }

    auto find(const KeyT& key) -> iterator
    {
      if (slot_count_ == 0) {
        return end();
      }
      const auto index = do_find_index(key);
      return iterator(metadatas_ + index, entries_ + index);
    }

    auto find(const KeyT& key) const -> const_iterator
    {
      if (slot_count_ == 0) {
        return end();
      }
      const auto index = do_find_index(key);
      return const_iterator(metadatas_ + index, entries_ + index);
    }

    template <typename QueryT>
      requires(BorrowTrait<KeyT, QueryT>::available)
    [[nodiscard]]
    auto entry_ref(QueryT key) -> EntryT&
    {
      SOUL_ASSERT(0, slot_count_ > 0, "");
      const auto index = do_find_index(key);
      return entries_[index];
    }

    template <typename QueryT>
      requires(BorrowTrait<KeyT, QueryT>::available)
    [[nodiscard]]
    auto entry_ref(QueryT key) const -> const EntryT&
    {
      SOUL_ASSERT(0, slot_count_ > 0, "");
      const auto index = do_find_index(key);
      return entries_[index];
    }

    [[nodiscard]]
    auto entry_ref(const KeyT& key) -> EntryT&
    {
      SOUL_ASSERT(0, slot_count_ > 0, "");
      const auto index = do_find_index(key);
      return entries_[index];
    }

    [[nodiscard]]
    auto entry_ref(const KeyT& key) const -> const EntryT&
    {
      SOUL_ASSERT(0, slot_count_ > 0, "");
      const auto index = do_find_index(key);
      return entries_[index];
    }

    template <typename QueryT>
      requires(BorrowTrait<KeyT, QueryT>::available)
    void remove(QueryT key)
    {
      if (slot_count() == 0) {
        return;
      }
      auto prev_bucket_index = do_find_index(key);
      do_remove_index(prev_bucket_index);
    }

    void remove(const KeyT& key)
    {
      if (slot_count() == 0) {
        return;
      }
      auto prev_bucket_index = do_find_index(key);
      do_remove_index(prev_bucket_index);
    }
  };
} // namespace soul
