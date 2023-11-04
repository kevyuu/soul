#pragma once

#include <filesystem>
#include "core/type_traits.h"
#include <type_traits>

namespace soul
{
  class Path : public std::filesystem::path
  {
  public:
    constexpr Path(Path&& other) = default;
    auto operator=(Path&& other) noexcept -> Path& = default;
    constexpr ~Path() = default;

    Path(std::filesystem::path&& path) // NOLINT
        : std::filesystem::path(std::move(path))
    {
    }

    [[nodiscard]]
    static auto From(const char* str) -> Path
    {
      return std::filesystem::path(str);
    }

    [[nodiscard]]
    auto clone() -> Path
    {
      return Path(*this);
    }

    auto clone_from(const Path& path) { *this = path; }

  private:
    Path(const Path& other) = default;
    auto operator=(const Path& other) -> Path& = default;
  };

} // namespace soul
