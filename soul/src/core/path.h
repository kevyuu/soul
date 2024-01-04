#pragma once

#include <filesystem>
#include "core/string.h"
#include "core/type_traits.h"

namespace soul
{
  class Path : public std::filesystem::path
  {
    using base_type = std::filesystem::path;

  public:
    constexpr Path(Path&& other)                   = default;
    auto operator=(Path&& other) noexcept -> Path& = default;
    constexpr ~Path()                              = default;

    [[nodiscard]]
    static auto From(StringView string_view) -> Path
    {
      return Path(std::filesystem::path(string_view.begin(), string_view.end()));
    }

    [[nodiscard]]
    auto clone() -> Path
    {
      return Path(*this);
    }

    auto clone_from(const Path& path)
    {
      *this = path;
    }

    auto operator/=(StringView str) -> Path&
    {
      base_type::append(str.begin(), str.end());
      return *this;
    }

    void clear() noexcept
    {
      return base_type::clear();
    }

    [[nodiscard]]
    auto canonical() const -> Path
    {
      return Path(std::filesystem::canonical(*this));
    }

    [[nodiscard]]
    auto root_name() const -> Path
    {
      return Path(base_type::root_name());
    }

    [[nodiscard]]
    auto root_directory() const -> Path
    {
      return Path(base_type::root_directory());
    }

    [[nodiscard]]
    auto relative_path() const -> Path
    {
      return Path(base_type::relative_path());
    }

    [[nodiscard]]
    auto parent_path() const -> Path
    {
      return Path(base_type::parent_path());
    }

    [[nodiscard]]
    auto filename() const -> Path
    {
      return Path(base_type::filename());
    }

    [[nodiscard]]
    auto stem() const -> Path
    {
      return Path(base_type::stem());
    }

    [[nodiscard]]
    auto extension() const -> Path
    {
      return Path(base_type::extension());
    }

    void swap(Path& other) noexcept
    {
      base_type::swap(other);
    }

    friend auto operator<=>(const Path& lhs, const Path& rhs) noexcept = default;

    friend auto operator/(const Path& lhs, const Path& rhs) -> Path
    {
      const base_type& lhs_base = lhs;
      const base_type& rhs_base = rhs;
      return Path(lhs_base / rhs_base);
    }

    friend auto operator/(const Path& lhs, StringView str) -> Path
    {
      const base_type& lhs_base = lhs;
      return Path(lhs / base_type(str.begin(), str.end()));
    }

  private:
    explicit Path(std::filesystem::path&& path) : std::filesystem::path(std::move(path)) {}

    Path(const Path& other) = default;

    auto operator=(const Path& other) -> Path& = default;
  };

} // namespace soul
