#include "textgen_backend.h"
#include "streaming_buffer.h"
#include "type.h"

#include "core/log.h"
#include "core/string_view.h"
#include "core/vector.h"

#include "misc/json.h"
#include "misc/string_util.h"

#include <curl/curl.h>
#include <curl/easy.h>

namespace khaos
{
  struct TextgenResponse
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
auto soul_op_construct_from_json<khaos::TextgenResponse::Choice>(JsonReadRef val_ref)
  -> khaos::TextgenResponse::Choice
{
  khaos::TextgenResponse::Choice choice;
  choice.index = val_ref.ref("index"_str).as_i32();
  choice.text  = String::From(val_ref.ref("text"_str).as_string_view());
  return choice;
}

template <>
auto soul_op_construct_from_json<khaos::TextgenResponse>(JsonReadRef val_ref)
  -> khaos::TextgenResponse
{
  khaos::TextgenResponse response;
  response.id = String::From(val_ref.ref("id"_str).as_string_view());
  val_ref.ref("choices"_str)
    .as_array_for_each(
      [&](u32 /*index*/, JsonReadRef choice_json_ref)
      {
        response.choices.push_back(
          soul_op_construct_from_json<khaos::TextgenResponse::Choice>(choice_json_ref));
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
    const auto full_response = StringView(ptr, nmemb);
    SOUL_LOG_INFO("Response : {}", full_response);
    auto* buffer = reinterpret_cast<StreamingBuffer*>(userdata);
    if (nmemb < 4)
    {
      return size * nmemb;
    }
    str::for_each_line(full_response, 
      [buffer](u64 line_i, StringView line)
      {
        if (StringView(line.data(), 4) == "data"_str)
        {
          const auto trimmed_response = str::trim(StringView(line.data() + 6, line.size() - 6));
          if (trimmed_response != "[DONE]"_str)
          {
            const auto response         = from_json_string<TextgenResponse>(trimmed_response);
            buffer->push(response.choices.back().text.cview());
          }
        }
      });
    return size * nmemb;
  }

  TextgenBackend::TextgenBackend() : handle_(curl_easy_init())
  {
    SOUL_ASSERT(0, handle_ != nullptr);
  }

  TextgenBackend::~TextgenBackend()
  {
    curl_easy_cleanup(handle_);
  }

  void TextgenBackend::request_streaming_completion(
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
