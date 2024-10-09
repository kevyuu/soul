#include "text_completion_api.h"

namespace khaos
{
  TextCompletionApi::TextCompletionApi(String url) : cli_(std::string(url.c_str())) {}

  auto TextCompletionApi::connect() -> Option<String>
  {
    auto res = cli_.Get("/v1/models");
    if (res->status != 200)
    {
      return nilopt;
    }
    return nilopt;
  }

} // namespace khaos
