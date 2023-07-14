#pragma once

namespace soul
{
  struct Uninitialized {
  };
  constexpr auto uninitialized = Uninitialized{};

  struct UninitializedDummy {
    // NOLINTBEGIN(hicpp-use-equals-default, modernize-use-equals-default)
    constexpr UninitializedDummy() noexcept {}
    // NOLINTEND(hicpp-use-equals-default, modernize-use-equals-default)
  };
} // namespace soul
