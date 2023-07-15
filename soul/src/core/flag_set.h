#pragma once

#include <numeric>

#include "core/bitset.h"
#include "core/type.h"

namespace soul
{

  template <ts_flag Flag>
  class FlagSet;

  template <class T>
  struct is_flag_set : std::false_type {
  };

  template <ts_flag Flag>
  struct is_flag_set<FlagSet<Flag>> : std::true_type {
  };

  template <typename T>
  concept flag_set = is_flag_set<T>::value;

  namespace impl
  {
    template <typename T>
    concept dst_flag = flag_set<T> || std::integral<T>;
  }

  template <ts_flag Flag>
  class FlagSet
  {
  public:
    using flag_type = Flag;
    static constexpr usize FLAG_COUNT = to_underlying(Flag::COUNT);
    using store_type = Bitset<FLAG_COUNT>;

    // Default constructor (all 0s)
    constexpr FlagSet() noexcept = default;
    constexpr FlagSet(const FlagSet& other) noexcept = default;
    constexpr auto operator=(const FlagSet& other) noexcept -> FlagSet& = default;
    constexpr FlagSet(FlagSet&& other) noexcept = default;
    constexpr auto operator=(FlagSet&& other) noexcept -> FlagSet& = default;
    constexpr ~FlagSet() noexcept = default;

    // Initializer list constructor
    constexpr FlagSet(std::initializer_list<flag_type> init_list);

    constexpr auto set() -> FlagSet&;
    constexpr auto set(flag_type bit, b8 value = true) -> FlagSet&;
    constexpr auto reset() -> FlagSet&;
    constexpr auto reset(flag_type bit) -> FlagSet&;
    constexpr auto flip() -> FlagSet&;
    constexpr auto flip(flag_type bit) -> FlagSet&;

    [[nodiscard]]
    constexpr auto
    operator[](flag_type bit) const -> b8;
    constexpr auto operator|=(FlagSet flag) -> FlagSet&;
    constexpr auto operator&=(FlagSet flag) -> FlagSet&;
    constexpr auto operator^=(FlagSet flag) -> FlagSet&;
    [[nodiscard]]
    constexpr auto
    operator~() const -> FlagSet;
    constexpr auto operator==(const FlagSet&) const -> b8 = default;

    [[nodiscard]]
    constexpr auto count() const -> size_t;
    [[nodiscard]]
    constexpr auto size() const -> size_t;
    [[nodiscard]]
    constexpr auto test(flag_type bit) const -> b8;
    [[nodiscard]]
    constexpr auto test_any(FlagSet other) const -> b8;
    [[nodiscard]]
    constexpr auto test_any(std::initializer_list<flag_type> other) const -> b8;
    [[nodiscard]]
    constexpr auto any() const -> b8;
    [[nodiscard]]
    constexpr auto none() const -> b8;

    template <usize IFlagCount = FLAG_COUNT>
      requires(IFlagCount <= 32)
    [[nodiscard]]
    constexpr auto to_uint32() const -> u32;

    template <usize IFlagCount = FLAG_COUNT>
      requires(IFlagCount <= 64)
    [[nodiscard]]
    constexpr auto to_uint64() const -> u64;

    template <impl::dst_flag DstFlags, usize N>
      requires(N == to_underlying(Flag::COUNT))
    [[nodiscard]]
    constexpr auto map(const DstFlags (&mapping)[N]) const -> DstFlags;

    template <ts_fn<void, Flag> T>
    constexpr auto for_each(T func) const -> void;

    template <ts_fn<b8, Flag> T>
    constexpr auto find_if(T func) const -> std::optional<Flag>;

    store_type flags_;

  private:
    explicit constexpr FlagSet(store_type flags) : flags_(flags) {}

    constexpr auto flags_from_init_list(std::initializer_list<flag_type> init_list) -> store_type;
    static constexpr usize MASK = (1u << FLAG_COUNT) - 1;
  };

  template <ts_flag Flag>
  constexpr FlagSet<Flag>::FlagSet(const std::initializer_list<flag_type> init_list)
  {
    for (const auto val : init_list) {
      flags_.set(to_underlying(val));
    }
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::size() const -> size_t
  {
    return to_underlying(Flag::COUNT);
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::test(flag_type bit) const -> b8
  {
    return flags_.test(to_underlying(bit));
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::test_any(FlagSet other) const -> b8
  {
    return (*this & other).any();
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::test_any(std::initializer_list<flag_type> other) const -> b8
  {
    return (*this & FlagSet(other)).any();
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::any() const -> b8
  {
    return flags_.any();
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::none() const -> b8
  {
    return flags_.none();
  }

  template <ts_flag Flag>
  template <usize IFlagCount>
    requires(IFlagCount <= 32)
  constexpr auto FlagSet<Flag>::to_uint32() const -> u32
  {
    return flags_.to_uint32();
  }

  template <ts_flag Flag>
  template <usize IFlagCount>
    requires(IFlagCount <= 64)
  constexpr auto FlagSet<Flag>::to_uint64() const -> u64
  {
    return flags_.to_uint64();
  }

  template <ts_flag Flag>
  template <impl::dst_flag DstFlags, usize N>
    requires(N == to_underlying(Flag::COUNT))
  constexpr auto FlagSet<Flag>::map(const DstFlags (&mapping)[N]) const -> DstFlags
  {
    std::remove_cv_t<DstFlags> dst_flags{};
    store_type flags = flags_;
    auto pos = flags_.find_first();
    while (pos) {
      dst_flags |= mapping[*pos];
      pos = flags_.find_next(*pos);
    }
    return dst_flags;
  }

  template <ts_flag Flag>
  template <ts_fn<void, Flag> T>
  constexpr auto FlagSet<Flag>::for_each(T func) const -> void
  {
    auto new_func = [func = std::move(func)](usize bit) { func(flag_type(bit)); };
    flags_.for_each(new_func);
  }

  template <ts_flag Flag>
  template <ts_fn<b8, Flag> T>
  constexpr auto FlagSet<Flag>::find_if(T func) const -> std::optional<Flag>
  {
    auto new_func = [func = std::move(func)](usize bit) -> b8 {
      return func(flag_type(bit));
    };
    const auto find_result = flags_.find_if(new_func);
    if (find_result) {
      return static_cast<Flag>(find_result.value());
    }
    return std::nullopt;
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::flags_from_init_list(
    const std::initializer_list<flag_type> init_list) -> store_type
  {
    store_type result;
    for (auto val : init_list) {
      result.set(to_underlying(val));
    }
    return result;
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::operator[](flag_type bit) const -> b8
  {
    return test(bit);
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::set() -> FlagSet<Flag>&
  {
    flags_.set();
    return *this;
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::set(flag_type bit, b8 value) -> FlagSet<Flag>&
  {
    flags_.set(to_underlying(bit), value);
    return *this;
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::reset() -> FlagSet<Flag>&
  {
    flags_.reset();
    return *this;
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::reset(flag_type bit) -> FlagSet<Flag>&
  {
    flags_.set(to_underlying(bit), false);
    return *this;
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::flip() -> FlagSet<Flag>&
  {
    flags_.flip();
    return *this;
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::flip(flag_type bit) -> FlagSet<Flag>&
  {
    flags_.flip(to_underlying(bit));
    return *this;
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::operator|=(FlagSet flag) -> FlagSet<Flag>&
  {
    flags_ |= flag.flags_;
    return *this;
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::operator&=(FlagSet flag) -> FlagSet<Flag>&
  {
    flags_ &= flag.flags_;
    return *this;
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::operator^=(FlagSet flag) -> FlagSet<Flag>&
  {
    flags_ ^= flag.flags_;
    return *this;
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::operator~() const -> FlagSet<Flag>
  {
    return FlagSet(~this->flags_);
  }

  template <ts_flag Flag>
  constexpr auto FlagSet<Flag>::count() const -> size_t
  {
    return flags_.count();
  }

  template <ts_flag FlagT>
  constexpr auto operator&(const FlagSet<FlagT> lhs, const FlagSet<FlagT> rhs) -> FlagSet<FlagT>
  {
    return FlagSet<FlagT>(lhs).operator&=(rhs);
  }

  template <ts_flag FlagT>
  constexpr auto operator|(const FlagSet<FlagT> lhs, const FlagSet<FlagT> rhs) -> FlagSet<FlagT>
  {
    return FlagSet<FlagT>(lhs).operator|=(rhs);
  }

  template <ts_flag FlagT>
  constexpr auto operator^(const FlagSet<FlagT> lhs, const FlagSet<FlagT> rhs) -> FlagSet<FlagT>
  {
    return FlagSet<FlagT>(lhs).operator^=(rhs);
  }

} // namespace soul
