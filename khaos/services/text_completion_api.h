#pragma once

#include "core/string.h"

#include "httplib.h"

using namespace soul;

namespace khaos
{
  class TextCompletionApi
  {
  public:
    explicit TextCompletionApi(String url);
    auto connect() -> Option<String>;

  private:
    httplib::Client cli_;
  };

} // namespace khaos
