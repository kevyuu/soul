#pragma once

#include <regex>
#include "core/builtins.h"
#include "core/option.h"
#include "core/own_ref.h"
#include "core/type_traits.h"

namespace soul
{

  template <typeset OkT, typeset ErrT>
  class Result;

  template <typename T, typename OkT = match_any, typename ErrT = match_any>
  inline constexpr b8 is_result_v = []() {
    if constexpr (!is_specialization_v<T, Result>) {
      return false;
    } else {
      constexpr b8 is_ok_type_match = is_match_v<OkT, typename T::ok_type>;
      constexpr b8 is_err_type_match = is_match_v<ErrT, typename T::err_type>;
      return is_ok_type_match && is_err_type_match;
    }
  }();

  template <typename T, typename OkT = match_any, typename ErrT = match_any>
  concept ts_result = is_result_v<T, OkT, ErrT>;

  template <typeset OkT, typeset ErrT>
  class Result
  {
    static inline constexpr auto can_result_trivial_copy_v =
      can_trivial_copy_v<OkT> && can_trivial_copy_v<ErrT>;

    static inline constexpr auto can_result_trivial_move_v =
      can_trivial_move_v<OkT> && can_trivial_move_v<ErrT>;

    static inline constexpr auto can_result_nontrivial_move_v =
      !can_result_trivial_move_v && can_move_v<OkT> && can_move_v<ErrT>;

    static inline constexpr auto can_result_trivial_destruct_v =
      can_trivial_destruct_v<OkT> && can_trivial_destruct_v<ErrT>;

    static inline constexpr auto can_result_clone_v =
      !can_result_trivial_copy_v && can_copy_or_clone_v<OkT> && can_copy_or_clone_v<ErrT>;

  public:
    using ok_type = OkT;
    using err_type = ErrT;

    constexpr Result(const Result& other) noexcept
      requires(can_result_trivial_copy_v)
    = default;

    constexpr Result(Result&& other) noexcept
      requires(can_result_trivial_move_v)
    = default;

    constexpr Result(Result&& other) noexcept
      requires(can_result_nontrivial_move_v)
        : state_(other.state_)
    {
      SOUL_ASSERT(0, other.state_ != State::VALUELESS, "Cannot move from a valueless object");
      if (state_ == State::OK) {
        relocate_at(&ok_val_, std::move(other.ok_val_));
      } else if (state_ == State::ERR) {
        relocate_at(&err_val_, std::move(other.err_val_));
      }
      other.state_ = State::VALUELESS;
    }

    constexpr auto operator=(const Result& other) noexcept -> Result&
      requires(can_result_trivial_copy_v)
    = default;

    constexpr auto operator=(Result&& other) noexcept -> Result&
      requires(can_result_trivial_move_v)
    = default;

    constexpr auto operator=(Result&& other) noexcept -> Result&
      requires(can_result_nontrivial_move_v)
    {
      SOUL_ASSERT(0, other.state_ != State::VALUELESS, "Cannot move from a valueless object");

      if (state_ == other.state_) {
        if (state_ == State::OK) {
          ok_val_ = std::move(other.ok_val_);
        } else {
          err_val_ = std::move(other.err_val_);
        }
        other.cleanup_for_not_valueless();
      } else {
        cleanup_for_not_valueless();
        state_ = other.state_;
        if (other.state_ == State::OK) {
          relocate_at(&ok_val_, std::move(other.ok_val_));
        } else if (other.state_ == State::ERR) {
          relocate_at(&err_val_, std::move(other.err_val_));
        }
      }
      other.state_ = State::VALUELESS;

      return *this;
    }

    constexpr ~Result() noexcept
      requires(can_result_trivial_destruct_v)
    = default;

    constexpr ~Result() noexcept
      requires(!can_result_trivial_destruct_v)
    {
      if (state_ != State::VALUELESS) {
        cleanup_for_not_valueless();
      }
    }

    friend constexpr void swap(Result& lhs, Result& rhs)
    {
      SOUL_ASSERT(
        0,
        lhs.state_ != State::VALUELESS && rhs.state_ != State::VALUELESS,
        "Cannot swap valueless result");
      using std::swap;
      if (lhs.state_ == rhs.state_) {
        if (lhs.state_ == State::OK) {
          swap(lhs.ok_val_, rhs.ok_val_);
        } else {
          swap(lhs.err_val_, rhs.err_val_);
        }
      } else {
        if (lhs.state_ == State::OK) {
          OkT ok_val_tmp = std::move(lhs.ok_val_);
          destroy_at(&lhs.ok_val_);
          relocate_at(&lhs.err_val_, std::move(rhs.err_val_));
          construct_at(&rhs.ok_val_, std::move(ok_val_tmp));
        } else {
          ErrT err_val_tmp = std::move(lhs.err_val_);
          destroy_at(&lhs.err_val_);
          relocate_at(&lhs.ok_val_, std::move(rhs.ok_val_));
          construct_at(&rhs.err_val_, std::move(err_val_tmp));
        }
        swap(lhs.state_, rhs.state_);
      }
    }

    [[nodiscard]]
    constexpr auto clone() const -> Result
      requires(can_result_clone_v)
    {
      return Result(*this);
    }

    constexpr void clone_from(const Result& other)
      requires(can_result_clone_v)
    {
      *this = other;
    }

    [[nodiscard]]
    static constexpr auto Ok(OwnRef<OkT> val) noexcept -> Result
    {
      return Result(Construct::ok, val.forward());
    }

    [[nodiscard]]
    static constexpr auto Err(OwnRef<ErrT> val) noexcept -> Result
    {
      return Result(Construct::err, val.forward());
    }

    template <ts_generate_fn<OkT> Fn>
    [[nodiscard]]
    static constexpr auto Generate(Fn fn) -> Result
    {
      return Result(Construct::init_generate, fn);
    }

    template <ts_generate_fn<ErrT> Fn>
    [[nodiscard]]
    static constexpr auto GenerateErr(Fn fn) -> Result
    {
      return Result(Construct::init_generate_err, fn);
    }

    [[nodiscard]]
    constexpr auto ok_ref() -> OkT&
    {
      return ok_val_;
    }

    [[nodiscard]]
    constexpr auto ok_ref() const -> const OkT&
    {
      return ok_val_;
    }

    [[nodiscard]]
    constexpr auto err_ref() -> ErrT&
    {
      return err_val_;
    }

    [[nodiscard]]
    constexpr auto err_ref() const -> const ErrT&
    {
      return err_val_;
    }

    [[nodiscard]]
    constexpr auto unwrap() const& -> OkT
    {
      static_assert(
        can_trivial_copy_v<OkT>,
        "Ok type must be trivially copyable to use transform on lvalue reference");
      SOUL_ASSERT(0, state_ == State::OK, "cannot unwrap non ok result");
      return ok_val_;
    }

    [[nodiscard]]
    constexpr auto unwrap() && -> OkT
    {
      SOUL_ASSERT(0, state_ == State::OK, "Cannot unwrap non ok result");
      return std::move(ok_val_);
    }

    [[nodiscard]]
    constexpr auto unwrap_or(OwnRef<OkT> default_val) const& -> OkT
    {
      static_assert(
        can_trivial_copy_v<OkT>,
        "Ok type must be trivially copyable to use transform on lvalue reference");
      if (is_ok()) {
        return ok_val_;
      }
      return default_val;
    }

    [[nodiscard]]
    constexpr auto unwrap_or(OwnRef<OkT> default_val) && -> OkT
    {
      if (is_ok()) {
        return std::move(ok_val_);
      }
      return default_val;
    }

    template <ts_fn<OkT> Fn>
    [[nodiscard]]
    constexpr auto unwrap_or_else(Fn fn) const& -> OkT
    {
      static_assert(
        can_trivial_copy_v<OkT>,
        "Ok type must be trivially copyable to use transform on lvalue reference");
      if (is_ok()) {
        return ok_val_;
      }
      return std::invoke(fn);
    }

    template <ts_fn<OkT> Fn>
    [[nodiscard]]
    constexpr auto unwrap_or_else(Fn fn) && -> OkT
    {
      if (is_ok()) {
        return std::move(ok_val_);
      }
      return std::invoke(fn);
    }

    template <ts_invocable<OkT&> Fn, typename FnReturnT = invoke_result_t<Fn, OkT&>>
    [[nodiscard]]
    constexpr auto and_then(Fn fn) & -> FnReturnT
      requires(is_result_v<FnReturnT, match_any, ErrT>)
    {
      static_assert(
        can_trivial_copy_v<ErrT>,
        "Error type must be trivially copyable to use transform on lvalue reference");
      if (is_ok()) {
        return std::invoke(fn, ok_val_);
      }
      return FnReturnT::Err(err_val_);
    }

    template <ts_invocable<const OkT&> Fn, typename FnReturnT = invoke_result_t<Fn, const OkT&>>
    [[nodiscard]]
    constexpr auto and_then(Fn fn) const& -> FnReturnT
      requires(is_result_v<FnReturnT, match_any, ErrT>)
    {
      static_assert(
        can_trivial_copy_v<ErrT>,
        "Error type must be trivially copyable to use transform on lvalue reference");
      if (is_ok()) {
        return std::invoke(fn, ok_val_);
      }
      return FnReturnT::Err(err_val_);
    }

    template <ts_invocable<OkT&&> Fn, typename FnReturnT = invoke_result_t<Fn, OkT&&>>
    [[nodiscard]]
    constexpr auto and_then(Fn fn) && -> FnReturnT
      requires(is_result_v<FnReturnT, match_any, ErrT>)
    {
      if (is_ok()) {
        return std::invoke(fn, std::move(ok_val_));
      }
      return FnReturnT::Err(std::move(err_val_));
    }

    template <typename Fn>
    [[nodiscard]]
    constexpr auto and_then(Fn fn) const&& = delete;

    template <ts_invocable<OkT&> Fn, typename FnReturnT = invoke_result_t<Fn, OkT&>>
    [[nodiscard]]
    constexpr auto transform(Fn fn) & -> Result<FnReturnT, ErrT>
    {
      static_assert(
        can_trivial_copy_v<ErrT>,
        "Error type must be trivially copyable to use transform on lvalue reference");
      using ReturnT = Result<FnReturnT, ErrT>;
      if (is_ok()) {
        return ReturnT::Generate([&, this] { return std::invoke(fn, ok_val_); });
      }
      return ReturnT::Err(err_val_);
    }

    template <ts_invocable<const OkT&> Fn, typename FnReturnT = invoke_result_t<Fn, const OkT&>>
    [[nodiscard]]
    constexpr auto transform(Fn fn) const& -> Result<FnReturnT, ErrT>
    {
      static_assert(
        can_trivial_copy_v<ErrT>,
        "Error type must be trivially copyable to use transform on lvalue reference");
      using ReturnT = Result<FnReturnT, ErrT>;
      if (is_ok()) {
        return ReturnT::Generate([&, this] { return std::invoke(fn, ok_val_); });
      }
      return ReturnT::Err(err_val_);
    }

    template <ts_invocable<OkT&&> Fn, typename FnReturnT = invoke_result_t<Fn, OkT&&>>
    [[nodiscard]]
    constexpr auto transform(Fn fn) && -> Result<FnReturnT, ErrT>
    {
      using ReturnT = Result<FnReturnT, ErrT>;
      if (is_ok()) {
        return ReturnT::Generate([&, this] { return std::invoke(fn, std::move(ok_val_)); });
      }
      return ReturnT::Err(std::move(err_val_));
    }

    template <typename Fn>
    [[nodiscard]]
    constexpr auto transform(Fn fn) const&& = delete;

    template <typeset Fn, ts_result<OkT, match_any> FnReturnT = invoke_result_t<Fn, ErrT&>>
    [[nodiscard]]
    constexpr auto or_else(Fn fn) & -> FnReturnT
    {
      static_assert(
        can_trivial_copy_v<OkT>,
        "Ok type must be trivially copyable to use this method on lvalue reference");
      if (is_ok()) {
        return FnReturnT::Ok(ok_val_);
      }
      return std::invoke(fn, err_val_);
    }

    template <typeset Fn, ts_result<OkT, match_any> FnReturnT = invoke_result_t<Fn, const ErrT&>>
    [[nodiscard]]
    constexpr auto or_else(Fn fn) const& -> FnReturnT
    {
      static_assert(
        can_trivial_copy_v<OkT>,
        "Ok type must be trivially copyable to use this method on lvalue reference");
      if (is_ok()) {
        return FnReturnT::Ok(ok_val_);
      }
      return std::invoke(fn, err_val_);
    }

    template <typeset Fn, ts_result<OkT, match_any> FnReturnT = invoke_result_t<Fn, ErrT&&>>
    [[nodiscard]]
    constexpr auto or_else(Fn fn) && -> FnReturnT
    {
      if (is_ok()) {
        return FnReturnT::Ok(std::move(ok_val_));
      }
      return std::invoke(fn, std::move(err_val_));
    }

    template <typename Fn>
    [[nodiscard]]
    constexpr auto or_else(Fn fn) const&& = delete;

    [[nodiscard]]
    constexpr auto is_ok() const -> b8
    {
      return state_ == State::OK;
    }

    template <ts_fn<b8, const OkT&> Fn>
    [[nodiscard]]
    constexpr auto is_ok_and(Fn fn) const
    {
      return state_ == State::OK && std::invoke(fn, ok_val_);
    }

    [[nodiscard]]
    constexpr auto is_err() const -> b8
    {
      return state_ == State::ERR;
    }

    template <ts_fn<b8, const ErrT&> Fn>
    [[nodiscard]]
    constexpr auto is_err_and(Fn fn) const -> b8
    {
      return state_ == State::ERR && std::invoke(fn, err_val_);
    }

  private:
    union {
      OkT ok_val_;
      ErrT err_val_;
    };
    enum class State : u8 { OK, ERR, VALUELESS, COUNT };
    State state_;

    struct Construct {
      struct Ok {
      };
      struct Err {
      };
      struct InitGenerate {
      };
      struct InitGenerateErr {
      };
      static constexpr auto ok = Ok{};
      static constexpr auto err = Err{};
      static constexpr auto init_generate = InitGenerate{};
      static constexpr auto init_generate_err = InitGenerateErr{};
    };

    constexpr explicit Result(Construct::Ok /* tag */, OwnRef<OkT> ok_val)
        : ok_val_(ok_val), state_(State::OK)
    {
    }

    constexpr explicit Result(Construct::Err /* tag */, OwnRef<ErrT> err_val)
        : err_val_(err_val), state_(State::ERR)
    {
    }

    template <ts_generate_fn<OkT> Fn>
    constexpr explicit Result(Construct::InitGenerate /* tag */, Fn fn) : state_(State::OK)
    {
      generate_at(&ok_val_, fn);
    }

    template <ts_generate_fn<ErrT> Fn>
    constexpr explicit Result(Construct::InitGenerateErr /* tag */, Fn fn) : state_(State::ERR)
    {
      generate_at(&err_val_, fn);
    }

    constexpr Result(const Result& other)
      requires(can_result_clone_v)
        : state_(other.state_)
    {
      SOUL_ASSERT(0, other.state_ != State::VALUELESS, "Cannot copy from valueless object");
      if (other.state_ == State::OK) {
        duplicate_at(&ok_val_, other.ok_val_);
      } else if (other.state_ == State::ERR) {
        duplicate_at(&err_val_, other.err_val_);
      }
    }

    constexpr auto operator=(const Result& other) noexcept -> Result&
      requires(can_result_clone_v)
    {
      SOUL_ASSERT(0, other.state_ != State::VALUELESS, "Cannot copy from valueless object");
      if (state_ == other.state_) {
        if (state_ == State::OK) {
          duplicate_from(ptrof(ok_val_), other.ok_val_);
        } else {
          duplicate_from(ptrof(err_val_), other.err_val_);
        }
      } else {
        cleanup_for_not_valueless();

        if (other.state_ == State::OK) {
          duplicate_at(&ok_val_, other.ok_val_);
        } else {
          duplicate_at(&err_val_, other.err_val_);
        }
        state_ = other.state_;
      }
      return *this;
    }

    constexpr void cleanup_for_not_valueless()
    {
      if (state_ == State::OK) {
        destroy_at(&ok_val_);
      } else if (state_ == State::ERR) {
        destroy_at(&err_val_);
      }
    }
  };

  template <typeset OkT, typeset ErrT>
  constexpr auto operator==(const Result<OkT, ErrT>& left, const Result<OkT, ErrT>& right) noexcept
    -> b8
    requires(can_compare_equality_v<OkT> && can_compare_equality_v<ErrT>)
  {
    if (left.is_ok() != right.is_ok()) {
      return false;
    }
    if (left.is_ok()) {
      return left.ok_ref() == right.ok_ref();
    }
    return left.err_ref() == right.err_ref();
  }
} // namespace soul
