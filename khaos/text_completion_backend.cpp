#include "text_completion_backend.h"
#include "misc/json.h"
#include "streaming_buffer.h"
#include "type.h"

#include "core/log.h"
#include "core/string_view.h"
#include "core/vector.h"

#include <string_view>

#include <curl/curl.h>
#include <curl/easy.h>

namespace
{

  auto trim(StringView view) -> StringView
  {
    auto s = std::string_view(view.begin(), view.size());
    s.remove_prefix(std::min(s.find_first_not_of(" \t\r\v\n"), s.size()));
    s.remove_suffix(std::min(s.size() - s.find_last_not_of(" \t\r\v\n") - 1, s.size()));
    return StringView(s.data(), cast<u64>(s.size()));
  }

} // namespace

namespace khaos
{
  struct TextCompletionResponse
  {
    struct Choice
    {
      i32 index;
      String text;
    };

    String id;
    Vector<Choice> choices;
  };
} // namespace khaos

template <>
auto soul_op_construct_from_json<khaos::TextCompletionResponse::Choice>(JsonReadRef val_ref)
  -> khaos::TextCompletionResponse::Choice
{
  khaos::TextCompletionResponse::Choice choice;
  choice.index = val_ref.ref("index"_str).as_i32();
  choice.text  = String::From(val_ref.ref("text"_str).as_string_view());
  return choice;
}

template <>
auto soul_op_construct_from_json<khaos::TextCompletionResponse>(JsonReadRef val_ref)
  -> khaos::TextCompletionResponse
{
  khaos::TextCompletionResponse response;
  response.id = String::From(val_ref.ref("id"_str).as_string_view());
  val_ref.ref("choices"_str)
    .as_array_for_each(
      [&](u32 /*index*/, JsonReadRef choice_json_ref)
      {
        response.choices.push_back(
          soul_op_construct_from_json<khaos::TextCompletionResponse::Choice>(choice_json_ref));
      });
  return response;
}

namespace khaos
{
  auto create_request_body_json(
    StringView prompt,
    const SamplerParameter& parameter,
    u32 max_token_count,
    StringView grammar_string,
    b8 streaming) -> String
  {
    JsonDoc doc;
    JsonObjectRef object_ref = doc.create_root_object(parameter);
    object_ref.add("prompt"_str, prompt);
    object_ref.add("max_tokens"_str, max_token_count);
    object_ref.add("grammar_string"_str, grammar_string);
    object_ref.add("stream"_str, streaming);
    return doc.dump();
  }

  auto streaming_write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t
  {
    SOUL_LOG_INFO("Response : {}", StringView(ptr, nmemb));
    auto* buffer = reinterpret_cast<StreamingBuffer*>(userdata);
    if (nmemb < 4)
    {
      return size * nmemb;
    }
    if (StringView(ptr, 4) == "data"_str)
    {
      const auto trimmed_response = trim(StringView(ptr + 6, nmemb - 6));
      const auto response         = from_json_string<TextCompletionResponse>(trimmed_response);
      buffer->push(response.choices.back().text.cspan());
    }
    return size * nmemb;
  }

  TextCompletionBackend::TextCompletionBackend() : handle_(curl_easy_init())
  {
    SOUL_ASSERT(0, handle_ != nullptr);
  }

  TextCompletionBackend::~TextCompletionBackend()
  {
    curl_easy_cleanup(handle_);
  }

  void TextCompletionBackend::request_streaming_completion(
    StreamingBuffer* buffer,
    StringView url,
    StringView prompt,
    const SamplerParameter& parameter,
    u32 max_token_count,
    StringView grammar_string)
  {
    String complete_url = String::Format("{}/v1/completions", url);
    SOUL_ASSERT(0, url.is_null_terminated());

    struct curl_slist* headers = NULL;
    headers                    = curl_slist_append(headers, "Accept: application/json");
    headers                    = curl_slist_append(headers, "Content-Type: application/json");
    headers                    = curl_slist_append(headers, "charset: utf-8");

    curl_easy_setopt(handle_, CURLOPT_URL, complete_url.data());
    curl_easy_setopt(handle_, CURLOPT_POST, 1L);
    curl_easy_setopt(handle_, CURLOPT_HTTPHEADER, headers);
    const auto request_body =
      create_request_body_json(prompt, parameter, max_token_count, grammar_string, true);
    SOUL_LOG_INFO("Request body : {}", request_body.c_str());
    curl_easy_setopt(handle_, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(handle_, CURLOPT_WRITEFUNCTION, streaming_write_callback);
    curl_easy_setopt(handle_, CURLOPT_WRITEDATA, reinterpret_cast<void*>(buffer));
    const auto result = curl_easy_perform(handle_);

    if (result != CURLE_OK)
    {
      SOUL_LOG_ERROR("curl_easy_perform() failed: {}", curl_easy_strerror(result));
    }
  }
} // namespace khaos
