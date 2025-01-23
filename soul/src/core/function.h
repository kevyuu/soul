#pragma once

#include "core/objops.h"
#include "core/type_traits.h"

namespace soul
{
  namespace impl
  {
    template <usize CapacityV>
    union AlignedStorageHelper
    {
      struct double1
      {
        double a;
      };

      struct double4
      {
        double a[4];
      };
      template <class T>
      using maybe = std::conditional_t<(CapacityV >= sizeof(T)), T, char>;
      char real_data[CapacityV];
      maybe<int> a;
      maybe<long> b;
      maybe<long long> c;
      maybe<void*> d;
      maybe<void (*)()> e;
      maybe<double1> f;
      maybe<double4> g;
      maybe<long double> h;
    };

    template <usize CapacityV, usize AlignV = alignof(AlignedStorageHelper<CapacityV>)>
    struct AlignedStorage
    {
      using type = std::aligned_storage_t<CapacityV, AlignV>;
    };

    template <usize CapacityV, usize AlignV = alignof(AlignedStorageHelper<CapacityV>)>
    using aligned_storage_t = typename AlignedStorage<CapacityV, AlignV>::type;
    static_assert(sizeof(aligned_storage_t<sizeof(void*)>) == sizeof(void*), "A");
    static_assert(alignof(aligned_storage_t<sizeof(void*)>) == alignof(void*), "B");

  } // namespace impl

  struct NilFunction
  {
  };

  constexpr auto nilfunction = NilFunction{};

  template <
    typename SignatureT,
    usize CapacityV  = 32,
    usize AlignmentV = alignof(impl::AlignedStorageHelper<CapacityV>)>
  class Function;

  template <typename ReturnT, typename... Args, usize CapacityV, usize AlignmentV>
  class Function<ReturnT(Args...), CapacityV, AlignmentV>
  {
  public:
    using invoke_ptr_t                             = ReturnT (*)(void*, Args&&...);
    static constexpr invoke_ptr_t nil_invoke_ptr_t = [](void*, Args&&... args) -> ReturnT {};

    Function(NilFunction nilfunction) : invoke_ptr_(nilfunction) {}

    template <typename T, typename ClosureT = std::decay_t<T>>
    Function(T&& closure)
        : invoke_ptr_(
            [](void* data, Args&&... args) -> ReturnT
            {
              ClosureT* closure = std::launder(reinterpret_cast<ClosureT*>(data));
              return (*closure)(std::forward<Args>(args)...);
            })
    {
      static_assert(
        ts_copy<ClosureT>,
        "Function cannot be constructed from non trivial type (type with notrivial copy and "
        "destructor).");

      static_assert(
        sizeof(ClosureT) <= CapacityV,
        "Function cannot be constructed from object with this (large) size.");

      static_assert(
        AlignmentV % alignof(ClosureT) == 0,
        "Function cannot be constructed from object with this (large alignment.");

      std::memcpy(buffer_, &closure, sizeof(ClosureT));
    }

    Function(const Function& other);
    Function(Function&& other)                         = default;
    auto operator=(const Function& other) -> Function& = default;
    auto operator=(Function&& other) -> Function&      = default;

    ~Function() = default;

    auto operator=(NilFunction) -> Function&
    {
      invoke_ptr_ = nilfunction;
      return *this;
    }

    auto operator==(NilFunction) const -> b8
    {
      return invoke_ptr_ == nilfunction;
    }

    auto operator()(Args... args) -> ReturnT
    {
      return invoke_ptr_(reinterpret_cast<void*>(buffer_), std::forward<Args>(args)...);
    }

  private:
    alignas(AlignmentV) std::byte buffer_[CapacityV];
    invoke_ptr_t invoke_ptr_;
  };
} // namespace soul
