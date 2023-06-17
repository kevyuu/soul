#pragma once

#include <ranges>

#include "core/objops.h"

namespace soul::views
{
  template <typename T>
  auto clone()
  {
    return std::views::transform([](const T& val) -> T { return val.clone(); });
  }

  template <typename T>
  auto clone_span(T* first, size_t size)
  {
    return std::span(first, size) | soul::views::clone<T>();
  }

  template <typename T>
  auto duplicate()
  {
    return std::views::transform([](const T& val) -> T { return soul::duplicate(val); });
  }

  template <typename T>
  auto duplicate_span(T* first, size_t size)
  {
    return std::span(first, size) | soul::views::duplicate<T>();
  }
} // namespace soul::views
