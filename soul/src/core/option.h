#pragma once

#include "core/objops.h"
#include "core/own_ref.h"
#include "core/type.h"
#include "core/type_traits.h"
#include "core/uninitialized.h"

namespace soul
{
  class NilOpt
  {
  };

  constexpr auto nilopt = NilOpt{};

  template <typeset T>
  class Option;

  template <typename T, typename SomeT = match_any>
  inline constexpr b8 is_option_v = []
  {
    if constexpr (is_specialization_v<T, Option>)
    {
      return is_match_v<SomeT, typename T::some_type>;
    }
    return false;
  }();

  template <typename T, typename SomeT = match_any>
  concept ts_option = is_option_v<T, SomeT>;

  namespace impl
  {

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
        return opt.is_some() && std::invoke(fn, opt.some_ref());
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
      constexpr auto unwrap_or(T default_val) const& -> val_ret_type
        requires(can_trivial_copy_v<Option<T>>)
      {
        auto& opt = get_option();
        if (opt.is_some())
        {
          return opt.some_ref();
        }
        return val_ret_type(default_val);
      }

      [[nodiscard]]
      constexpr auto unwrap_or(T default_val) && -> val_ret_type
      {
        auto& opt = get_option();
        if (opt.is_some())
        {
          return std::move(opt.some_ref());
        }
        return default_val;
      }

      template <ts_invocable Fn>
      [[nodiscard]]
      constexpr auto unwrap_or_else(Fn fn) const& -> val_ret_type
        requires(can_trivial_copy_v<Option<T>>)
      {
        static_assert(
          ts_convertible_to<invoke_result_t<Fn>, val_ret_type>,
          "fn return type need to be convertible to Option::some_type");
        auto& opt = get_option();
        return opt.is_some() ? opt.some_ref() : static_cast<val_ret_type>(std::invoke(fn));
      }

      template <ts_invocable Fn>
      [[nodiscard]]
      constexpr auto unwrap_or_else(Fn fn) && -> val_ret_type
      {
        static_assert(
          ts_convertible_to<invoke_result_t<Fn>, val_ret_type>,
          "fn return type need to be convertible to Option::some_type");
        auto& opt = get_option();
        return opt.is_some() ? std::move(opt.some_ref()) : std::invoke(fn);
      }

      template <typeset Fn, ts_option FnReturnT = std::invoke_result_t<Fn, T&>>
      [[nodiscard]]
      constexpr auto and_then(Fn fn) & -> FnReturnT
      {
        auto& opt = get_option();
        if (opt.is_some())
        {
          return std::invoke(fn, opt.some_ref());
        }
        return FnReturnT{};
      }

      template <typeset Fn, ts_option FnReturnT = std::invoke_result_t<Fn, const T&>>
      [[nodiscard]]
      constexpr auto and_then(Fn fn) const& -> FnReturnT
      {
        const auto& opt = get_option();
        if (opt.is_some())
        {
          return std::invoke(fn, opt.some_ref());
        }
        return FnReturnT{};
      }

      template <typeset Fn, ts_option FnReturnT = std::invoke_result_t<Fn, T&&>>
      [[nodiscard]]
      constexpr auto and_then(Fn fn) && -> FnReturnT
      {
        auto& opt = get_option();
        if (opt.is_some())
        {
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
        if (opt.is_some())
        {
          return Option<FnReturnT>::Generate(
            [&, this]
            {
              return std::invoke(fn, opt.some_ref());
            });
        }
        return Option<FnReturnT>();
      }

      template <typeset Fn, typename FnReturnT = std::invoke_result_t<Fn, const T&>>
      [[nodiscard]]
      constexpr auto transform(Fn fn) const& -> Option<FnReturnT>
      {
        const auto& opt = get_option();
        if (opt.is_some())
        {
          return Option<FnReturnT>::Generate(
            [&, this]
            {
              return std::invoke(fn, opt.some_ref());
            });
        }
        return Option<FnReturnT>();
      }

      template <typeset Fn, typename FnReturnT = std::invoke_result_t<Fn, T&&>>
      [[nodiscard]]
      constexpr auto transform(Fn fn) && -> Option<FnReturnT>
      {
        auto& opt = get_option();
        if (opt.is_some())
        {
          return Option<FnReturnT>::Generate(
            [&, this]
            {
              return std::invoke(fn, std::move(opt.some_ref()));
            });
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
        if (opt.is_some())
        {
          return opt;
        }
        return std::invoke(fn);
      }

      template <ts_fn<Option<T>> Fn>
      [[nodiscard]]
      constexpr auto or_else(Fn fn) && -> Option<T>
      {
        auto& opt = get_option();
        if (opt.is_some())
        {
          return std::move(opt);
        }
        return std::invoke(fn);
      }

    private:
      [[nodiscard]]
      constexpr auto get_option() -> Option<T>&
      {
        return static_cast<Option<T>&>(*this);
      }

      [[nodiscard]]
      constexpr auto get_option() const -> const Option<T>&
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
    union
    {
      UninitializedDummy dummy_;
      std::remove_const_t<T> value_;
    };

    b8 is_some_ = false;

    using val_ret_type = std::remove_cv_t<T>;

  public:
    using some_type = T;

    constexpr Option() noexcept : dummy_() {}

    constexpr Option(NilOpt none_val) : dummy_() {} // NOLINT

    constexpr Option(const Option& other) noexcept
      requires(can_trivial_copy_v<T>)
    = default;

    constexpr Option(Option&& other) noexcept
      requires(can_trivial_move_v<T>)
    = default;

    constexpr Option(Option&& other) noexcept
      requires(can_nontrivial_move_v<T>);

    constexpr Option(const T& val) noexcept // NOLINT(hicpp-explicit-conversions)
      requires(can_trivial_copy_v<T>);

    constexpr Option(T&& val) noexcept // NOLINT(hicpp-explicit-conversions)
      requires(!can_trivial_copy_v<T>);

    constexpr auto operator=(const Option& other) noexcept -> Option&
      requires(can_trivial_copy_v<T>)
    = default;

    constexpr auto operator=(Option&& other) noexcept -> Option&
      requires(can_trivial_move_v<T>)
    = default;

    constexpr auto operator=(Option&& other) noexcept -> Option&
      requires(can_nontrivial_move_v<T>);

    constexpr auto operator=(const T& val) noexcept // NOLINT(hicpp-explicit-conversions)
      -> Option&
      requires(can_trivial_copy_v<T>);

    constexpr auto operator=(T&& val) noexcept // NOLINT(hicpp-explicit-conversions)
      -> Option&
      requires(!can_trivial_copy_v<T>);

    constexpr ~Option() noexcept
      requires(can_trivial_destruct_v<T>)
    = default;
    constexpr ~Option() noexcept
      requires(can_nontrivial_destruct_v<T>);

    [[nodiscard]]
    static constexpr auto Some(OwnRef<T> val) noexcept -> Option;

    template <ts_generate_fn<T> Fn>
    [[nodiscard]]
    static constexpr auto Generate(Fn fn) noexcept -> Option;

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
      if (is_some_)
      {
        duplicate_at(&value_, other.value_);
      }
    }

    constexpr auto operator=(const Option& other) -> Option&
      requires(can_clone_v<T> || can_nontrivial_copy_v<T>)
    {
      Option(other).swap(*this);
      return *this;
    }

    struct Construct
    {
      struct Some
      {
      };

      struct InitGenerate
      {
      };

      static constexpr auto some          = Some{};
      static constexpr auto init_generate = InitGenerate{};
    };

    constexpr explicit Option(Construct::Some /* tag */, OwnRef<T> val) : is_some_(true)
    {
      construct_at(&value_, std::move(val));
    }

    template <ts_generate_fn<T> Fn>
    constexpr explicit Option(Construct::InitGenerate /* tag */, Fn fn) : is_some_(true)
    {
      generate_at(&value_, fn);
    }
  };

  template <typeset T>
  constexpr Option<T>::Option(Option&& other) noexcept
    requires(can_nontrivial_move_v<T>)
      : is_some_(other.is_some_)
  {
    if (other.is_some_)
    {
      relocate_at(&value_, std::move(other.value_));
      other.is_some_ = false;
    }
  }

  template <typeset T>
  constexpr Option<T>::Option(const T& val) noexcept
    requires(can_trivial_copy_v<T>)
      : is_some_(true)
  {
    construct_at(&value_, val);
  }

  template <typeset T>
  constexpr Option<T>::Option(T&& val) noexcept
    requires(!can_trivial_copy_v<T>)
      : is_some_(true)
  {
    construct_at(&value_, std::move(val));
  }

  template <typeset T>
  constexpr auto Option<T>::operator=(Option&& other) noexcept -> Option&
    requires(can_nontrivial_move_v<T>)
  {
    if (other.is_some_)
    {
      if (is_some_)
      {
        value_ = std::move(other.value_);
      } else
      {
        is_some_ = true;
        construct_at(&value_, std::move(other.value_));
      }
    } else
    {
      reset();
    }
    return *this;
  }

  template <typeset T>
  constexpr auto Option<T>::operator=(const T& val) noexcept -> Option&
    requires(can_trivial_copy_v<T>)
  {
    if (is_some_)
    {
      value_ = val;
    } else
    {
      is_some_ = true;
      construct_at(&value_, val);
    }
    return *this;
  }

  template <typeset T>
  constexpr auto Option<T>::operator=(T&& val) noexcept -> Option&
    requires(!can_trivial_copy_v<T>)
  {
    if (is_some_)
    {
      value_ = std::move(val);
    } else
    {
      is_some_ = true;
      construct_at(&value_, std::move(val));
    }
    return *this;
  }

  template <typeset T>
  constexpr Option<T>::~Option() noexcept
    requires(can_nontrivial_destruct_v<T>)
  {
    if (is_some_)
    {
      destroy_at(&value_);
    }
  }

  template <typeset T>
  [[nodiscard]]
  constexpr auto Option<T>::Some(OwnRef<T> val) noexcept -> Option
  {
    return Option(Construct::some, std::move(val));
  }

  template <typeset T>
  template <ts_generate_fn<T> Fn>
  [[nodiscard]]
  constexpr auto Option<T>::Generate(Fn fn) noexcept -> Option
  {
    return Option(Construct::init_generate, fn);
  }

  template <typeset T>
  constexpr void Option<T>::swap(Option& other) noexcept
    requires(can_move_v<T> && can_swap_v<T>)
  {
    if (is_some_ == other.is_some_)
    {
      if (is_some_)
      {
        using std::swap;
        swap(value_, other.value_);
      }
    } else
    {
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
    if (is_some_)
    {
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
    if (left.is_some() && right.is_some())
    {
      return left.some_ref() == right.some_ref();
    }
    return left.is_some() == right.is_some();
  }

  template <typeset T>
  constexpr auto operator==(const Option<T>& left, NilOpt /* nilopt */) noexcept -> b8
  {
    return !left.is_some();
  }

  template <typeset T>
  constexpr auto someopt(const T& val) -> Option<T>
  {
    return Option<T>::Some(val);
  };

  template <typeset T>
  constexpr auto someopt(T&& val) -> Option<T>
  {
    return Option<T>::Some(std::move(val)); // NOLINT
  };

  template <ts_pointer T>
  class Option<NotNull<T>> : public impl::OptionBase<NotNull<T>>
  {
  public:
    using some_type = NotNull<T>;
    using ptr_type  = T;

    constexpr Option() : not_null_ptr_(NotNull<T>::NewUnchecked(nullptr)) {}

    constexpr Option(NilOpt none) : not_null_ptr_(NotNull<T>::NewUnchecked(nullptr)) {} // NOLINT

    constexpr Option(T ptr) : not_null_ptr_(NotNull<T>::NewUnchecked(ptr)) {} // NOLINT

    constexpr Option(NotNull<T>) = delete;

    constexpr Option(const Option&) = default;

    constexpr Option(Option&&) noexcept = default;

    constexpr auto operator=(const Option&) -> Option& = default;

    constexpr auto operator=(Option&&) noexcept -> Option& = default;

    ~Option() = default;

    constexpr operator T() const
    {
      return not_null_ptr_.get_unchecked();
    } // NOLINT

    constexpr operator NotNull<T>() const = delete;

    constexpr void swap(Option& other)
    {
      not_null_ptr_.swap(other.not_null_ptr_);
    }

    [[nodiscard]]
    static constexpr auto Some(NotNull<T> not_null_ptr) noexcept -> Option
    {
      T ptr = not_null_ptr;
      return Option(ptr);
    }

    template <ts_fn<NotNull<T>> Fn>
    [[nodiscard]]
    static constexpr auto Generate(Fn fn) noexcept -> Option
    {
      return Option::Some(std::invoke(fn));
    }

    [[nodiscard]]
    constexpr auto some_ref() -> NotNull<T>&
    {
      return not_null_ptr_;
    }

    [[nodiscard]]
    constexpr auto some_ref() const -> const NotNull<T>&
    {
      return not_null_ptr_;
    }

    [[nodiscard]]
    constexpr auto is_some() const -> b8
    {
      return not_null_ptr_.get_unchecked() != nullptr;
    }

    constexpr void reset()
    {
      not_null_ptr_.set_unchecked(nullptr);
    }

    [[nodiscard]]
    constexpr auto unwrap_or(NotNull<T> default_val) const -> NotNull<T>
    {
      if (is_some())
      {
        return not_null_ptr_;
      }
      return default_val;
    }

    auto operator++() -> Option&              = delete;
    auto operator--() -> Option&              = delete;
    auto operator++(int) -> Option            = delete;
    auto operator--(int) -> Option            = delete;
    auto operator+=(std::ptrdiff_t) -> Option = delete;
    auto operator-=(std::ptrdiff_t) -> Option = delete;
    void operator[](std::ptrdiff_t) const     = delete;

  private:
    NotNull<T> not_null_ptr_;
  };

  template <typename T, typename PtrT = match_any>
  inline constexpr b8 is_maybe_null_v = []
  {
    if constexpr (is_option_v<T>)
    {
      return is_not_null_v<typename T::some_type, PtrT>;
    }
    return false;
  }();

  template <typename T, typename PtrT = match_any>
  concept ts_maybe_null = is_maybe_null_v<T, PtrT>;

  template <ts_pointer T>
  using MaybeNull = Option<NotNull<T>>;

  // more unwanted operators
  template <ts_pointer T, ts_pointer U>
  auto operator-(const MaybeNull<T>&, const MaybeNull<U>&) -> std::ptrdiff_t = delete;
  template <ts_pointer T>
  auto operator-(const MaybeNull<T>&, std::ptrdiff_t) -> MaybeNull<T> = delete;
  template <ts_pointer T>
  auto operator+(const MaybeNull<T>&, std::ptrdiff_t) -> MaybeNull<T> = delete;
  template <ts_pointer T>
  auto operator+(std::ptrdiff_t, const MaybeNull<T>&) -> MaybeNull<T> = delete;

  template <typeset T>
  constexpr auto operator<=>(const MaybeNull<T>& lhs, const MaybeNull<T>& rhs) noexcept
  {
    return operator<=>(lhs.get(), rhs.get());
  }
} // namespace soul
