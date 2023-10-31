#pragma once

#include "core/builtins.h"
#include "core/meta.h"
#include "core/objops.h"
#include "core/own_ref.h"
#include "core/type_traits.h"

namespace soul
{
  namespace impl
  {
    template <usize I, typename ValueT>
    class TupleLeaf
    {

    public:
      constexpr TupleLeaf() = default;

      template <typename Arg>
        requires(std::is_constructible_v<ValueT, Arg &&>)
      constexpr explicit TupleLeaf(Arg&& arg) : value_(std::forward<Arg>(arg)) // NOLINT
      {
      }

      [[nodiscard]]
      constexpr auto clone() const -> TupleLeaf
        requires(can_clone_v<ValueT>)
      {
        return TupleLeaf(value_.clone());
      }

      [[nodiscard]]
      constexpr auto clone_from(const TupleLeaf& other)
        requires(can_clone_v<ValueT>)
      {
        value_.clone_from(other.value_);
      }

      [[nodiscard]]
      constexpr auto ref() -> ValueT&
      {
        return value_;
      }

      [[nodiscard]]
      constexpr auto ref() const -> const ValueT&
      {
        return value_;
      }

    private:
      ValueT value_;
    };

    template <usize IndexV, typename ValueT>
      requires(std::is_empty_v<ValueT>)
    class TupleLeaf<IndexV, ValueT> : public ValueT
    {
    public:
      constexpr TupleLeaf() = default;

      template <typename Arg>
        requires(std::is_constructible_v<ValueT, Arg &&>)
      constexpr explicit TupleLeaf(Arg&& arg) : ValueT(std::forward<Arg>(arg)) // NOLINT
      {
      }

      [[nodiscard]]
      constexpr auto clone() const -> TupleLeaf
        requires(can_clone_v<ValueT>)
      {
        return TupleLeaf(ValueT::clone());
      }

      [[nodiscard]]
      constexpr auto clone_from(const TupleLeaf& other)
        requires(can_clone_v<ValueT>)
      {
        ValueT::clone_from(static_cast<const ValueT&>(other));
      }

      [[nodiscard]]
      constexpr auto ref() -> ValueT&
      {
        return static_cast<ValueT&>(*this);
      }

      [[nodiscard]]
      constexpr auto ref() const -> const ValueT&
      {
        return static_cast<const ValueT&>(*this);
      }
    };

    template <typename IndexSequenceT, typename... Ts>
    class TupleStorage;

    template <usize... IndexVs, typename... Ts>
    class TupleStorage<std::index_sequence<IndexVs...>, Ts...> : public TupleLeaf<IndexVs, Ts>...
    {
    public:
      static constexpr b8 can_tuple_default_construct = (can_default_construct_v<Ts> && ...);
      static constexpr b8 can_tuple_trivial_copy = (can_trivial_copy_v<Ts> && ...);
      static constexpr b8 can_tuple_trivial_move = (can_trivial_move_v<Ts> && ...);
      static constexpr b8 can_tuple_clone =
        !can_tuple_trivial_copy && (can_copy_or_clone_v<Ts> && ...);
      static constexpr b8 can_tuple_nontrivial_move =
        !can_tuple_trivial_move && (can_move_v<Ts> && ...);
      static constexpr b8 can_tuple_trivial_destruct = (can_trivial_destruct_v<Ts> && ...);
      static constexpr b8 can_tuple_nontrivial_destruct = (can_nontrivial_destruct_v<Ts> || ...);
      static constexpr b8 can_tuple_swap = (can_swap_v<Ts> && ...);

      constexpr TupleStorage() = default;

      template <typename... ValueTs>
      constexpr explicit TupleStorage(
        std::index_sequence<IndexVs...> /*indexes*/, ValueTs&&... values)
          : TupleLeaf<IndexVs, Ts>(std::forward<ValueTs>(values))...
      {
      }

      constexpr TupleStorage(const TupleStorage& other)
        requires(can_tuple_trivial_copy)
      = default;

      constexpr TupleStorage(TupleStorage&& other) noexcept = default;

      constexpr auto operator=(const TupleStorage& other) -> TupleStorage&
        requires(can_tuple_trivial_copy)
      = default;

      constexpr auto operator=(TupleStorage&& other) noexcept -> TupleStorage& = default;

      constexpr ~TupleStorage() = default;

      [[nodiscard]]
      constexpr auto clone() const -> TupleStorage
        requires(can_tuple_clone)
      {
        return TupleStorage(
          std::make_index_sequence<sizeof...(Ts)>(), duplicate(ref<IndexVs>())...);
      }

      constexpr void clone_from(const TupleStorage& other)
        requires(can_tuple_clone)
      {
        (duplicate_from(
           NotNull(static_cast<TupleLeaf<IndexVs, Ts>*>(this)),
           static_cast<const TupleLeaf<IndexVs, Ts>&>(other)),
         ...);
      }

      template <usize IndexV>
      [[nodiscard]]
      constexpr auto ref() const -> const get_type_at_t<IndexV, Ts...>&
      {
        using BaseT = const TupleLeaf<IndexV, get_type_at_t<IndexV, Ts...>>*;
        BaseT base = this;
        return base->ref();
      }

      template <usize IndexV>
      [[nodiscard]]
      constexpr auto ref() -> get_type_at_t<IndexV, Ts...>&
      {
        using BaseT = TupleLeaf<IndexV, get_type_at_t<IndexV, Ts...>>*;
        BaseT base = this;
        return base->ref();
      }

      friend constexpr void swap(TupleStorage& lhs, TupleStorage& rhs) noexcept
        requires(can_tuple_swap)
      {
        using std::swap;
        (swap(lhs.ref<IndexVs>(), rhs.ref<IndexVs>()), ...);
      }

      [[nodiscard]]
      friend constexpr auto
      operator==(const TupleStorage& lhs, const TupleStorage& rhs) -> b8
        requires(can_compare_equality_v<Ts> && ...)
      {
        return ((lhs.ref<IndexVs>() == rhs.ref<IndexVs>()) && ...);
      }
    };
  }; // namespace impl

  template <typename T, typename... Ts>
  class Tuple
  {
  private:
    using storage = impl::TupleStorage<std::make_index_sequence<sizeof...(Ts) + 1>, T, Ts...>;
    storage storage_;

  public:
    constexpr Tuple() = default;

    constexpr Tuple(OwnRef<T> arg, OwnRef<Ts>... args) // NOLINT
        : storage_(
            std::make_index_sequence<sizeof...(Ts) + 1>(), arg.forward_ref(), args.forward_ref()...)
    {
    }

    template <usize IndexV>
    [[nodiscard]]
    constexpr auto ref() const -> const get_type_at_t<IndexV, T, Ts...>&
    {
      return storage_.template ref<IndexV>();
    }

    template <usize IndexV>
    [[nodiscard]]
    constexpr auto ref() -> get_type_at_t<IndexV, T, Ts...>&
    {
      return storage_.template ref<IndexV>();
    }

    [[nodiscard]]
    constexpr auto clone() const -> Tuple
      requires(can_clone_v<storage>)
    {
      return Tuple(storage_.clone());
    }

    constexpr void clone_from(const Tuple& other)
      requires(can_clone_v<storage>)
    {
      storage_.clone_from(other.storage_);
    }

    friend constexpr void swap(Tuple& lhs, Tuple& rhs) noexcept
      requires can_swap_v<storage>
    {
      return swap(lhs.storage_, rhs.storage_);
    }

    [[nodiscard]]
    friend constexpr auto
    operator==(const Tuple& lhs, const Tuple& rhs) -> b8
      requires(can_compare_equality_v<storage>)
    {
      return lhs.storage_ == rhs.storage_;
    }

  private:
    constexpr explicit Tuple(storage&& other_storage) : storage_(std::move(other_storage)) {}
  };

  template <class... ValueTs>
  Tuple(ValueTs...) -> Tuple<ValueTs...>;
} // namespace soul
