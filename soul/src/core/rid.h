#pragma once

#include "core/builtins.h"

namespace soul
{
  template <typename TagT, u8 GenerationBitCountV = 24>
  class RID
  {
  private:
    u64 id_;

    static constexpr u64 NULLID = std::numeric_limits<u64>::max();

    static constexpr u64 INDEX_BIT_COUNT = 64 - GenerationBitCountV;
    static constexpr u64 INDEX_MASK      = (1LLU << INDEX_BIT_COUNT) - 1;

    static constexpr u64 GENERATION_BIT_COUNT = GenerationBitCountV;
    static constexpr u64 GENERATION_MASK      = (1 << GENERATION_BIT_COUNT) - 1;

  public:
    RID() : id_(NULLID) {}

    static auto Create(u64 index, u64 generation)
    {
      return RID((generation << INDEX_BIT_COUNT) | index);
    }

    static auto Null() -> RID
    {
      return RID(NULLID);
    }

    [[nodiscard]]
    auto index() const -> u64
    {
      return id_ & INDEX_MASK;
    }

    [[nodiscard]]
    auto generation() const -> u64
    {
      return (id_ >> INDEX_BIT_COUNT) & GENERATION_MASK;
    }

    [[nodiscard]] [[nodiscard]]
    auto is_null() const -> b8
    {
      return id_ == NULLID;
    }

    auto operator==(const RID& other) const -> b8 = default;

    [[nodiscard]]
    auto to_underlying() -> u64
    {
      return id_;
    }

    friend void soul_op_hash_combine(auto& hasher, const RID& val)
    {
      hasher.combine(val.id_);
    }

  private:
    explicit RID(u64 id) : id_(id){};
  };
} // namespace soul
