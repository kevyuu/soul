#pragma once

#include "core/hash.h"
#include "core/meta.h"
#include "core/own_ref.h"
#include "core/type.h"
#include "core/type_traits.h"
#include "core/uninitialized.h"

namespace soul
{
  template <typename... Ts> // (7)
  struct VisitorSet : Ts... {
    using Ts::operator()...;
  };
  template <class... Ts>
  VisitorSet(Ts...) -> VisitorSet<Ts...>;

  namespace impl
  {

    template <b8 can_all_trivial_destruct, std::size_t index, typeset... Ts>
    union RecursiveUnion;

    template <b8 can_all_trivial_destruct, std::size_t index>
    union RecursiveUnion<can_all_trivial_destruct, index> {
    };

    template <b8 can_all_trivial_destruct, std::size_t index, typename T_first, typename... Ts>
    union RecursiveUnion<can_all_trivial_destruct, index, T_first, Ts...> {

      UninitializedDummy dummy_;
      remove_cv_t<T_first> first_;
      RecursiveUnion<can_all_trivial_destruct, index + 1, Ts...> remaining_;

      constexpr RecursiveUnion(const RecursiveUnion&) = default;
      constexpr RecursiveUnion(RecursiveUnion&&) noexcept = default;
      constexpr RecursiveUnion(Uninitialized /* tag */) : dummy_{} {} // NOLINT
      constexpr auto operator=(const RecursiveUnion&) -> RecursiveUnion& = default;
      constexpr auto operator=(RecursiveUnion&&) noexcept -> RecursiveUnion& = default;

      template <typename T>
      constexpr explicit RecursiveUnion(OwnRef<T> other)
        requires(is_same_v<T, T_first>)
          : first_(other)
      {
      }

      template <typename T>
      constexpr explicit RecursiveUnion(OwnRef<T> other)
        requires(!is_same_v<T, T_first>)
          : remaining_(other.forward())
      {
      }

      constexpr ~RecursiveUnion()
        requires(can_all_trivial_destruct)
      = default;
      constexpr ~RecursiveUnion()
        requires(!can_all_trivial_destruct)
      {
      }

      template <typename T>
      [[nodiscard]]
      constexpr auto ref() -> T&
      {
        if constexpr (is_same_v<T, T_first>) {
          return first_;
        } else {
          return remaining_.template ref<T>();
        }
      }

      template <typename T>
      [[nodiscard]]
      constexpr auto ref() const -> const T&
      {
        if constexpr (is_same_v<T, T_first>) {
          return first_;
        } else {
          return remaining_.template ref<T>();
        }
      }
    };

    template <typename... Ts>
    static inline b8 constexpr assert_can_variant_clone_v = [] {
      constexpr b8 can_variant_trivial_copy = (can_trivial_copy_v<Ts> && ...);
      static_assert(
        !can_variant_trivial_copy,
        "Variant is a trivial copy type since all variant alternative types is a trivial copy "
        "type. Trivial variant cannot clone");
      static_assert(
        (can_copy_or_clone_v<Ts> && ...),
        "T cannot copy or clone, Not all variant alternative types can copy or clone, so variant "
        "cannot clone");
      return true;
    }();

  } // namespace impl

  template <typeset... Ts>
    requires(!has_duplicate_type_v<Ts...>)
  class Variant
  {
    using T0 = get_type_at_t<0, Ts...>;

  public:
    static constexpr auto type_count = sizeof...(Ts);
    using index_type = typename std::conditional < type_count < (std::numeric_limits<u8>::max()), u8
      ,
          typename std::conditional<
            type_count<(std::numeric_limits<u16>::max)(), u16, u32>::type>::type;
    static constexpr index_type none_index = std::numeric_limits<index_type>::max();

    static_assert(type_count < none_index);

    static constexpr b8 can_variant_trivial_copy = (can_trivial_copy_v<Ts> && ...);
    static constexpr b8 can_variant_trivial_move = (can_trivial_move_v<Ts> && ...);
    static constexpr b8 can_variant_clone =
      !can_variant_trivial_copy && (can_copy_or_clone_v<Ts> && ...);
    static constexpr b8 can_variant_nontrivial_move =
      !can_variant_trivial_move && (can_move_v<Ts> && ...);
    static constexpr b8 can_variant_trivial_destruct = (can_trivial_destruct_v<Ts> && ...);
    static constexpr b8 can_variant_nontrivial_destruct = (can_nontrivial_destruct_v<Ts> || ...);
    static constexpr b8 can_variant_hash_combine = (impl_soul_op_hash_combine_v<Ts> && ...);

    template <typeset T>
    static inline b8 constexpr is_variant_alt_v = get_type_count_v<T, Ts...> == 1;

    template <typeset T>
    static inline b8 constexpr assert_is_variant_alt_v = [] {
      static_assert(is_variant_alt_v<T>, "T is not part of variant");
      return true;
    }();

    template <size_t index>
    using type_at = get_type_at_t<index, Ts...>;

    constexpr Variant(const Variant& other)
      requires(can_variant_trivial_copy)
    = default;

    constexpr Variant(Variant&& other) noexcept
      requires(can_variant_trivial_move)
    = default;

    constexpr Variant(Variant&& other) noexcept
      requires(can_variant_nontrivial_move)
        : storage_(uninitialized), active_index_(other.active_index_)
    {
      other.visit([this]<typeset VariantAltT>(VariantAltT& val) -> void {
        static_assert(get_type_count_v<VariantAltT, Ts...> == 1, "Variant type is not found");
        relocate_at(&this->ref<VariantAltT>(), std::move(val));
      });
      other.active_index_ = none_index;
    }

    constexpr auto operator=(const Variant& other) -> Variant&
      requires(can_variant_trivial_copy)
    = default;

    constexpr auto operator=(Variant&& other) noexcept -> Variant&
      requires(can_variant_trivial_move)
    = default;

    constexpr auto operator=(Variant&& other) noexcept -> Variant&
      requires(can_variant_nontrivial_move)
    {
      Variant(std::move(other)).swap(*this);
      return *this;
    }

    constexpr auto operator=(const Variant&& other) = delete;

    constexpr auto operator=(const Variant& other) const = delete;

    constexpr ~Variant()
      requires(can_variant_trivial_destruct)
    = default;

    constexpr ~Variant()
      requires(can_variant_nontrivial_destruct)
    {
      if (active_index_ != none_index) {
        cleanup_for_not_none();
      }
    }

    struct Construct {
      struct From {
      };
      static constexpr auto from = From{};
      struct Clone {
      };
    };

    template <typeset T>
    [[nodiscard]]
    static constexpr auto From(const T& val) -> Variant
      requires(can_trivial_copy_v<T> && assert_is_variant_alt_v<T>)
    {
      return Variant(Construct::from, OwnRef<T>(val));
    }

    template <typeset T>
    [[nodiscard]]
    static constexpr auto From(T&& val) -> Variant
      requires(assert_is_variant_alt_v<T>)
    {
      return Variant(Construct::from, OwnRef<T>(std::move(val))); // NOLINT
    }

    constexpr auto swap(Variant& other) noexcept
    {
      if (other.active_index_ == this->active_index_) {
        if (this->active_index_ != none_index) {
          other.visit([&lhs = *this]<typeset VariantAltT>(VariantAltT& val) -> void {
            using std::swap;
            swap(val, lhs.template ref<VariantAltT>());
          });
        }
      } else if (other.active_index_ == none_index) {
        other.visit([&lhs = *this]<typeset VariantAltT>(VariantAltT& val) -> void {
          lhs.assign_for_none(std::move(val));
        });
        cleanup_for_not_none();
      } else if (this->active_index_ == none_index) {
        this->visit([&other]<typeset VariantAltT>(VariantAltT& val) -> void {
          other.assign_for_none(std::move(val));
        });
        other.cleanup_for_not_none();
      } else {
        this->visit([&rhs = other, &lhs = *this]<typeset ThisAltT>(ThisAltT& lhs_val) -> void {
          rhs.visit([&rhs, &lhs, &lhs_val]<typeset OtherAltT>(OtherAltT& rhs_val) -> void {
            auto tmp = std::move(lhs_val);
            lhs.cleanup_for_not_none();
            lhs.assign_for_none(std::move(rhs_val));
            rhs.cleanup_for_not_none();
            rhs.assign_for_none(std::move(tmp));
          });
        });
      }
    }

    friend void swap(Variant& lhs, Variant& rhs) noexcept { return lhs.swap(rhs); }

    [[nodiscard]]
    constexpr auto clone() const -> Variant
      requires(impl::assert_can_variant_clone_v<Ts...>)
    {
      return Variant(*this);
    }

    constexpr auto clone_from(const Variant& other)
      requires(impl::assert_can_variant_clone_v<Ts...>)
    {
      *this = other;
    }

    template <typeset T>
    [[nodiscard]]
    constexpr auto has_value() const -> b8
      requires(assert_is_variant_alt_v<T>)
    {
      return get_type_index_v<T, Ts...> == active_index_;
    }

    template <typeset T>
    [[nodiscard]]
    constexpr auto ref() -> T&
      requires(assert_is_variant_alt_v<T>)
    {
      return storage_.template ref<T>();
    }

    template <typeset T>
    [[nodiscard]]
    constexpr auto ref() const -> const T&
      requires(assert_is_variant_alt_v<T>)
    {
      return storage_.template ref<T>();
    }

    template <typeset T>
    [[nodiscard]]
    constexpr auto unwrap() && -> T
      requires(assert_is_variant_alt_v<T>)
    {
      return std::move(storage_.template ref<T>());
    }

    template <typeset T>
    constexpr void assign(const T& val)
    {
      cleanup_for_not_none();
      assign_for_none(val);
    }

    template <typeset T>
    constexpr void assign(T&& val)
      requires(assert_is_variant_alt_v<T>)
    {
      cleanup_for_not_none();
      assign_for_none(std::move(val)); // NOLINT
    }

    template <ts_visitor<Ts&...> Fn, typename ReturnT = invoke_result_t<Fn, T0&>>
    constexpr auto visit(Fn fn) & -> ReturnT
    {
      return visit_switch<0, ReturnT>(*this, fn);
    }

    template <ts_visitor<const Ts&...> Fn, typename ReturnT = invoke_result_t<Fn, const T0&>>
    constexpr auto visit(Fn fn) const& -> ReturnT
    {
      return visit_switch<0, ReturnT>(*this, fn);
    }

    template <ts_visitor<Ts&&...> Fn, typename ReturnT = invoke_result_t<Fn, T0&&>>
    constexpr auto visit(Fn fn) && -> ReturnT
    {
      return visit_switch<0, ReturnT>(std::move(*this), fn);
    }

    template <ts_visitor<const Ts&&...> Fn, typename ReturnT = invoke_result_t<Fn, const T0&&>>
    constexpr auto visit(Fn fn) const&& -> ReturnT
    {
      return visit_switch<0, ReturnT>(std::move(*this), fn);
    }

  private:
    using storage = impl::RecursiveUnion<can_variant_trivial_destruct, 0, Ts...>;
    storage storage_;
    u8 active_index_;

    constexpr Variant(const Variant& other)
      requires(can_variant_clone)
        : storage_(uninitialized), active_index_(other.active_index_)
    {
      other.visit([this]<typeset VariantAltT>(const VariantAltT& val) -> void {
        static_assert(get_type_count_v<VariantAltT, Ts...> == 1, "Variant type is not found");
        duplicate_at(&this->ref<VariantAltT>(), val);
      });
    }

    template <typeset T>
    constexpr explicit Variant(Construct::From /* tag */, OwnRef<T> val)
      requires(is_variant_alt_v<T>)
        : storage_(val.forward()), active_index_(get_type_index_v<T, Ts...>)
    {
    }

    constexpr auto operator=(const Variant& other) -> Variant&
      requires(can_variant_clone)
    {
      Variant(other).swap(*this);
      return *this;
    }

    template <size_t case_index, typename ReturnT, typename Fn, typename Self>
    constexpr static auto visit_case(Self&& self, const Fn& fn) -> ReturnT
    {
      if constexpr (case_index < type_count) {
        if constexpr (is_lvalue_reference_v<Self>) {
          return std::invoke(fn, self.template ref<type_at<case_index>>());
        } else {
          return std::invoke(fn, std::move(self.template ref<type_at<case_index>>()));
        }
      } else {
        unreachable();
      }
    }

    template <size_t index_offset, typename ReturnT, typename Fn, typename Self>
    constexpr static auto visit_switch(Self&& self, const Fn& fn) -> ReturnT
    {
      if constexpr (index_offset < type_count) {
        switch (self.active_index_) {
        case index_offset + 0:
          return visit_case<index_offset + 0, ReturnT>(std::forward<Self>(self), fn);
        case index_offset + 1:
          return visit_case<index_offset + 1, ReturnT>(std::forward<Self>(self), fn);
        case index_offset + 2:
          return visit_case<index_offset + 2, ReturnT>(std::forward<Self>(self), fn);
        case index_offset + 3:
          return visit_case<index_offset + 3, ReturnT>(std::forward<Self>(self), fn);
        case index_offset + 4:
          return visit_case<index_offset + 4, ReturnT>(std::forward<Self>(self), fn);
        case index_offset + 5:
          return visit_case<index_offset + 5, ReturnT>(std::forward<Self>(self), fn);
        case index_offset + 6:
          return visit_case<index_offset + 6, ReturnT>(std::forward<Self>(self), fn);
        case index_offset + 7:
          return visit_case<index_offset + 7, ReturnT>(std::forward<Self>(self), fn);
        case index_offset + 8:
          return visit_case<index_offset + 8, ReturnT>(std::forward<Self>(self), fn);
        case index_offset + 9:
          return visit_case<index_offset + 9, ReturnT>(std::forward<Self>(self), fn);
        case index_offset + 10:
          return visit_case<index_offset + 10, ReturnT>(std::forward<Self>(self), fn);
        case index_offset + 11:
          return visit_case<index_offset + 11, ReturnT>(std::forward<Self>(self), fn);
        case index_offset + 12:
          return visit_case<index_offset + 12, ReturnT>(std::forward<Self>(self), fn);
        case index_offset + 13:
          return visit_case<index_offset + 13, ReturnT>(std::forward<Self>(self), fn);
        case index_offset + 14:
          return visit_case<index_offset + 14, ReturnT>(std::forward<Self>(self), fn);
        case index_offset + 15:
          return visit_case<index_offset + 15, ReturnT>(std::forward<Self>(self), fn);
        default: return visit_switch<index_offset + 16, ReturnT>(std::forward<Self>(self), fn);
        }
      } else {
        unreachable();
      }
    }

    template <typeset T>
      requires(can_trivial_copy_v<T>)
    constexpr void assign_for_none(const T& val)
    {
      std::construct_at(&ref<T>(), val); // NOLINT
      active_index_ = get_type_index_v<T, Ts...>;
    }

    template <typeset T>
      requires(can_nontrivial_move_v<T>)
    constexpr void assign_for_none(T&& val)
    {
      std::construct_at(&ref<T>(), std::move(val)); // NOLINT
      active_index_ = get_type_index_v<T, Ts...>;
    }

    constexpr void cleanup_for_not_none()
    {
      visit([](auto& val) -> void { destroy_at(&val); });
      active_index_ = none_index;
    }

    [[nodiscard]]
    friend constexpr auto
    operator==(const Variant& lhs, const Variant& rhs) -> b8
      requires(can_compare_equality_v<Ts> && ...)
    {
      return (lhs.active_index_ == rhs.active_index_) &&
             lhs.visit([&rhs]<typeset VariantAltT>(const VariantAltT& lhs_val) {
               return lhs_val == rhs.ref<VariantAltT>();
             });
    }

    friend constexpr void soul_op_hash_combine(auto& hasher, const Variant& variant)
    {
      hasher.combine(variant.active_index_);
      variant.visit([&hasher](const auto& val) { hasher.combine(val); });
    }
  };

} // namespace soul
