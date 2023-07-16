#pragma once

#include "core/objops.h"
#include "core/own_ref.h"
#include "core/panic.h"
#include "core/type.h"
#include "core/type_traits.h"
#include "core/uninitialized.h"
#include "core/util.h"

#include <optional>

namespace soul
{
  class None
  {
  };
  constexpr auto none = None{};

  template <typeset T>
  class Option;

  template <typename T, typename SomeT = match_any>
  inline constexpr b8 is_option_v =
    [] { return is_specialization_v<T, Option> && is_match_v<SomeT, typename T::some_type>; }();

  template <typename T, typename SomeT = match_any>
  concept ts_option = is_option_v<T, SomeT>;

  namespace impl
  {
    struct OptionConstruct {
      struct Some {
      };
      struct InitGenerate {
      };
      static constexpr auto some = Some{};
      static constexpr auto init_generate = InitGenerate{};
    };

    template <typename T>
    class OptionBase
    {
    public:
      using val_ret_type = std::remove_cv_t<T>;

      template <ts_fn<b8, const T&> Fn>
      [[nodiscard]]
      constexpr auto is_some_and(Fn fn) const -> b8
      {
        const auto& opt = get_option();
        if (opt.is_some()) {
          return true;
        }
        return std::invoke(fn, opt.some_ref());
      }

      [[nodiscard]]
      constexpr auto unwrap() const& -> val_ret_type
      {
        return get_option().some_ref();
      }

      [[nodiscard]]
      constexpr auto unwrap() && -> val_ret_type
      {
        return std::move(get_option().some_ref());
      }

      [[nodiscard]]
      constexpr auto unwrap_or(OwnRef<T> default_val) const& -> val_ret_type
        requires(can_trivial_copy_v<Option<T>>)
      {
        auto& opt = get_option();
        if (opt.is_some()) {
          return opt.some_ref();
        }
        return default_val;
      }

      [[nodiscard]]
      constexpr auto unwrap_or(OwnRef<T> default_val) && -> val_ret_type
      {
        auto& opt = get_option();
        if (opt.is_some()) {
          return std::move(opt.some_ref());
        }
        return default_val;
      }

      template <ts_fn<std::remove_cv_t<T>> Fn>
      [[nodiscard]]
      constexpr auto unwrap_or_else(Fn fn) const& -> val_ret_type
        requires(can_trivial_copy_v<Option<T>>)
      {
        auto& opt = get_option();
        return opt.is_some() ? opt.some_ref() : std::invoke(fn);
      }

      template <ts_fn<std::remove_cv_t<T>> Fn>
      [[nodiscard]]
      constexpr auto unwrap_or_else(Fn fn) && -> val_ret_type
      {
        auto& opt = get_option();
        return opt.is_some() ? std::move(opt.some_ref()) : std::invoke(fn);
      }

      template <typeset Fn, ts_option FnReturnT = std::invoke_result_t<Fn, T&>>
      [[nodiscard]]
      constexpr auto and_then(Fn fn) & -> FnReturnT
      {
        auto& opt = get_option();
        if (opt.is_some()) {
          return std::invoke(fn, opt.some_ref());
        }
        return FnReturnT{};
      }

      template <typeset Fn, ts_option FnReturnT = std::invoke_result_t<Fn, const T&>>
      [[nodiscard]]
      constexpr auto and_then(Fn fn) const& -> FnReturnT
      {
        const auto& opt = get_option();
        if (opt.is_some()) {
          return std::invoke(fn, opt.some_ref());
        }
        return FnReturnT{};
      }

      template <typeset Fn, ts_option FnReturnT = std::invoke_result_t<Fn, T&&>>
      [[nodiscard]]
      constexpr auto and_then(Fn fn) && -> FnReturnT
      {
        auto& opt = get_option();
        if (opt.is_some()) {
          return std::invoke(fn, std::move(opt.some_ref()));
        }
        return FnReturnT{};
      }

      template <typename Fn>
      [[nodiscard]]
      constexpr auto and_then(Fn fn) const&& = delete;

      template <typeset Fn, typename FnReturnT = std::invoke_result_t<Fn, T&>>
      [[nodiscard]]
      constexpr auto transform(Fn fn) & -> Option<FnReturnT>
      {
        auto& opt = get_option();
        if (opt.is_some()) {
          return Option<FnReturnT>::init_generate(
            [&, this] { return std::invoke(fn, opt.some_ref()); });
        }
        return Option<FnReturnT>();
      }

      template <typeset Fn, typename FnReturnT = std::invoke_result_t<Fn, const T&>>
      [[nodiscard]]
      constexpr auto transform(Fn fn) const& -> Option<FnReturnT>
      {
        const auto& opt = get_option();
        if (opt.is_some()) {
          return Option<FnReturnT>::init_generate(
            [&, this] { return std::invoke(fn, opt.some_ref()); });
        }
        return Option<FnReturnT>();
      }

      template <typeset Fn, typename FnReturnT = std::invoke_result_t<Fn, T&&>>
      [[nodiscard]]
      constexpr auto transform(Fn fn) && -> Option<FnReturnT>
      {
        auto& opt = get_option();
        if (opt.is_some()) {
          return Option<FnReturnT>::init_generate(
            [&, this] { return std::invoke(fn, std::move(opt.some_ref())); });
        }
        return Option<FnReturnT>();
      }

      template <typename Fn>
      [[nodiscard]]
      constexpr auto transform(Fn fn) const&& = delete;

      template <ts_fn<Option<T>> Fn>
      [[nodiscard]]
      constexpr auto or_else(Fn fn) const& -> Option<T>
        requires(can_trivial_copy_v<Option<T>>)
      {
        auto& opt = get_option();
        if (opt.is_some()) {
          return opt;
        }
        return std::invoke(fn);
      }

      template <ts_fn<Option<T>> Fn>
      [[nodiscard]]
      constexpr auto or_else(Fn fn) && -> Option<T>
      {
        auto& opt = get_option();
        if (opt.is_some()) {
          return std::move(opt);
        }
        return std::invoke(fn);
      }

    private:
      [[nodiscard]]
      auto get_option() -> Option<T>&
      {
        return static_cast<Option<T>&>(*this);
      }

      [[nodiscard]]
      auto get_option() const -> const Option<T>&
      {
        return static_cast<const Option<T>&>(*this);
      }

      friend Option<T>;
    };
  } // namespace impl

  template <typeset T>
  class Option : public impl::OptionBase<T>
  {

  private:
    union {
      UninitializedDummy dummy_;
      std::remove_const_t<T> value_;
    };
    b8 is_some_ = false;

    using val_ret_type = std::remove_cv_t<T>;

  public:
    using some_type = T;

    constexpr Option() noexcept : dummy_() {}

    constexpr Option(None none_val) : dummy_() {} // NOLINT

    constexpr Option(const Option& other) noexcept
      requires(can_trivial_copy_v<T>)
    = default;

    constexpr Option(Option&& other) noexcept
      requires(can_trivial_move_v<T>)
    = default;

    constexpr Option(Option&& other) noexcept
      requires(can_nontrivial_move_v<T>);

    constexpr auto operator=(const Option& other) noexcept -> Option&
      requires(can_trivial_copy_v<T>)
    = default;

    constexpr auto operator=(Option&& other) noexcept -> Option&
      requires(can_trivial_move_v<T>)
    = default;

    constexpr auto operator=(Option&& other) noexcept -> Option&
      requires(can_nontrivial_move_v<T>);

    constexpr ~Option() noexcept
      requires(can_trivial_destruct_v<T>)
    = default;
    constexpr ~Option() noexcept
      requires(can_nontrivial_destruct_v<T>);

    [[nodiscard]]
    static constexpr auto some(OwnRef<T> val) noexcept -> Option;

    template <ts_generate_fn<T> Fn>
    [[nodiscard]]
    static constexpr auto init_generate(Fn fn) noexcept -> Option;

    constexpr void swap(Option& other) noexcept
      requires(can_move_v<T> && can_swap_v<T>);

    [[nodiscard]]
    constexpr auto clone() const noexcept -> Option
      requires(can_clone_v<T>);

    constexpr void clone_from(const Option& other) noexcept
      requires(can_clone_v<T>);

    [[nodiscard]]
    constexpr auto some_ref() -> T&
    {
      return value_;
    }

    [[nodiscard]]
    constexpr auto some_ref() const -> const T&
    {
      return value_;
    }

    [[nodiscard]]
    constexpr auto is_some() const -> b8;

    constexpr void reset();

  private:
    constexpr Option(const Option& other)
      requires(can_clone_v<T> || can_nontrivial_copy_v<T>)
        : is_some_(other.is_some_)
    {
      if (is_some_) {
        duplicate_at(&value_, other.value_);
      }
    }

    constexpr auto operator=(const Option& other) -> Option&
      requires(can_clone_v<T> || can_nontrivial_copy_v<T>)
    {
      Option(other).swap(*this);
      return *this;
    }

    constexpr explicit Option(impl::OptionConstruct::Some /* tag */, OwnRef<T> val) : is_some_(true)
    {
      val.store_at(&value_);
    }

    template <ts_generate_fn<T> Fn>
    constexpr explicit Option(impl::OptionConstruct::InitGenerate /* tag */, Fn fn) : is_some_(true)
    {
      generate_at(&value_, fn);
    }
  };

  template <typeset T>
  constexpr Option<T>::Option(Option&& other) noexcept
    requires(can_nontrivial_move_v<T>)
      : is_some_(other.is_some_)
  {
    if (other.is_some_) {
      construct_at(&value_, std::move(other.value_));
      other.is_some_ = false;
    }
  }

  template <typeset T>
  constexpr auto Option<T>::operator=(Option&& other) noexcept -> Option&
    requires(can_nontrivial_move_v<T>)
  {
    if (other.is_some_) {
      if (is_some_) {
        value_ = std::move(other.value_);
      } else {
        is_some_ = true;
        construct_at(&value_, std::move(other.value_));
      }
    } else {
      reset();
    }
    return *this;
  }

  template <typeset T>
  constexpr Option<T>::~Option() noexcept
    requires(can_nontrivial_destruct_v<T>)
  {
    if (is_some_) {
      destroy_at(&value_);
    }
  }

  template <typeset T>
  [[nodiscard]]
  constexpr auto Option<T>::some(OwnRef<T> val) noexcept -> Option
  {
    return Option(impl::OptionConstruct::some, val.forward());
  }

  template <typeset T>
  template <ts_generate_fn<T> Fn>
  [[nodiscard]]
  constexpr auto Option<T>::init_generate(Fn fn) noexcept -> Option
  {
    return Option(impl::OptionConstruct::init_generate, fn);
  }

  template <typeset T>
  constexpr void Option<T>::swap(Option& other) noexcept
    requires(can_move_v<T> && can_swap_v<T>)
  {
    if (is_some_ == other.is_some_) {
      if (is_some_) {
        using std::swap;
        swap(value_, other.value_);
      }
    } else {
      auto& src = is_some_ ? *this : other;
      auto& dst = is_some_ ? other : *this;
      construct_at(&dst.value_, std::move(src.value_));
      dst.is_some_ = true;
      src.reset();
    }
  }

  template <typeset T>
  constexpr auto Option<T>::clone() const noexcept -> Option
    requires(can_clone_v<T>)
  {
    return Option(*this);
  }

  template <typeset T>
  constexpr void Option<T>::clone_from(const Option& other) noexcept
    requires(can_clone_v<T>)
  {
    *this = other;
  }

  template <typeset T>
  constexpr auto Option<T>::is_some() const -> b8
  {
    return is_some_;
  }

  template <typeset T>
  constexpr void Option<T>::reset()
  {
    if (is_some_) {
      destroy_at(&value_);
      is_some_ = false;
    }
  }

  template <typeset T>
  constexpr void swap(Option<T>& left, Option<T>& right) noexcept
    requires(can_move_v<T> && can_swap_v<T>)
  {
    left.swap(right);
  }

  template <typeset T>
  constexpr auto operator==(const Option<T>& left, const Option<T>& right) noexcept -> b8
  {
    if (left.is_some() && right.is_some()) {
      return left.some_ref() == right.some_ref();
    }
    return left.is_some() == right.is_some();
  }

} // namespace soul
