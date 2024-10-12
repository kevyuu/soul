#pragma once

#include "core/comp_str.h"
#include "core/string.h"
#include "core/string_view.h"

#include "core/type_traits.h"
#include "yyjson.h"

namespace soul
{
  enum class JsonType : u8
  {
    NONE,
    NIL,
    BOOL,
    NUMBER,
    STRING,
    ARRAY,
    OBJECT,
    COUNT
  };

  class JsonReadRef
  {
  public:
    explicit JsonReadRef(yyjson_val* val_ptr);

    [[nodiscard]]
    auto get_type() const -> JsonType;

    [[nodiscard]]
    auto ref(StringView key) const -> JsonReadRef;

    [[nodiscard]]
    auto as_raw() const -> StringView;

    [[nodiscard]]
    auto as_string_view() const -> StringView;

    [[nodiscard]]
    auto as_i32() const -> i32;

    [[nodiscard]]
    auto as_i64() const -> i64;

    [[nodiscard]]
    auto as_u32() const -> u32;

    [[nodiscard]]
    auto as_u64() const -> u64;

    [[nodiscard]]
    auto as_f32() const -> f32;

    [[nodiscard]]
    auto as_f64() const -> f64;

    [[nodiscard]]
    auto as_b8() const -> b8;

    [[nodiscard]]
    auto as_string_view_or(StringView default_val) const -> StringView;

    [[nodiscard]]
    auto as_i32_or(i32 default_val) const -> i32;

    [[nodiscard]]
    auto as_i64_or(i64 default_val) const -> i64;

    [[nodiscard]]
    auto as_u64_or(u64 default_val) const -> u64;

    [[nodiscard]]
    auto as_f64_or(f64 default_val) const -> f64;

    [[nodiscard]]
    auto as_b8_or(b8 default_val) const -> b8;

    template <ts_fn<void, u64, JsonReadRef> Fn>
    void as_array_for_each(Fn fn)
    {
      yyjson_val* hit = nullptr;
      u32 max         = 0;
      u32 idx         = 0;
      yyjson_arr_foreach(val_ptr_, idx, max, hit)
      {
        fn(idx, JsonReadRef(hit));
      }
    }

    template <ts_fn<void, StringView, JsonReadRef> Fn>
    void as_object_for_each(Fn fn)
    {
      yyjson_val* key = nullptr;
      yyjson_val* val = nullptr;
      u32 max         = 0;
      u32 idx         = 0;
      yyjson_obj_foreach(val_ptr_, idx, max, key, val)
      {
        fn(JsonReadRef(key).as_string_view(), JsonReadRef(val));
      }
    }

  private:
    yyjson_val* val_ptr_;
  };

  class JsonRef
  {
  public:
    friend class JsonDoc;
    friend class JsonObjectRef;
    friend class JsonArrayRef;

  private:
    yyjson_mut_val* val_;

    explicit JsonRef(yyjson_mut_val* val) : val_(val) {}

    [[nodiscard]]
    auto get_val() const -> yyjson_mut_val*
    {
      return val_;
    }
  };

  class JsonObjectRef
  {
  public:
    void add(CompStr key, JsonRef val)
    {
      yyjson_mut_obj_add_val(doc_, val_, key.c_str(), val.get_val());
    }

    void add(CompStr key, StringView val)
    {
      yyjson_mut_obj_add_strncpy(doc_, val_, key.c_str(), val.begin(), val.size());
    }

    void add(CompStr key, i32 val)
    {
      yyjson_mut_obj_add_int(doc_, val_, key.c_str(), val);
    }

    void add(CompStr key, u32 val)
    {
      yyjson_mut_obj_add_uint(doc_, val_, key.c_str(), val);
    }

    void add(CompStr key, i64 val)
    {
      yyjson_mut_obj_add_sint(doc_, val_, key.c_str(), val);
    }

    void add(CompStr key, u64 val)
    {
      yyjson_mut_obj_add_uint(doc_, val_, key.c_str(), val);
    }

    operator JsonRef() const // NOLINT
    {
      return JsonRef(val_);
    }

    friend class JsonDoc;

  private:
    yyjson_mut_doc* doc_;
    yyjson_mut_val* val_;

    explicit JsonObjectRef(yyjson_mut_doc* doc, yyjson_mut_val* val) : doc_(doc), val_(val) {}

    [[nodiscard]]
    auto get_val() const -> yyjson_mut_val*
    {
      return val_;
    }
  };

  class JsonArrayRef
  {
  public:
    void append(JsonRef val)
    {
      yyjson_mut_arr_add_val(val_, val.get_val());
    }

    operator JsonRef() const // NOLINT
    {
      return JsonRef(val_);
    }

    friend class JsonDoc;

  private:
    yyjson_mut_val* val_;

    explicit JsonArrayRef(yyjson_mut_val* val) : val_(val) {}
  };

  class JsonDoc
  {
  public:
    explicit JsonDoc() : doc_(yyjson_mut_doc_new(nullptr)) {}

    JsonDoc(const JsonDoc&)                    = delete;
    JsonDoc(JsonDoc&&)                         = delete;
    auto operator=(const JsonDoc&) -> JsonDoc& = delete;
    auto operator=(JsonDoc&&) -> JsonDoc&      = delete;

    ~JsonDoc()
    {
      yyjson_mut_doc_free(doc_);
    }

    [[nodiscard]]
    auto create_string(StringView str) -> JsonRef
    {
      return JsonRef(yyjson_mut_strn(doc_, str.begin(), str.size()));
    }

    [[nodiscard]]
    auto create_i32(i32 val) -> JsonRef
    {
      return JsonRef(yyjson_mut_int(doc_, val));
    }

    [[nodiscard]]
    auto create_u32(u32 val) -> JsonRef
    {
      return JsonRef(yyjson_mut_uint(doc_, val));
    }

    [[nodiscard]]
    auto create_i64(i64 val) -> JsonRef
    {
      return JsonRef(yyjson_mut_sint(doc_, val));
    }

    [[nodiscard]]
    auto create_u64(u64 val) -> JsonRef
    {
      return JsonRef(yyjson_mut_uint(doc_, val));
    }

    [[nodiscard]]
    auto create_real(f64 val) -> JsonRef
    {
      return JsonRef(yyjson_mut_real(doc_, val));
    }

    [[nodiscard]]
    auto create_empty_object() -> JsonObjectRef
    {
      return JsonObjectRef(doc_, yyjson_mut_obj(doc_));
    }

    [[nodiscard]]
    auto create_empty_array() -> JsonArrayRef
    {
      return JsonArrayRef(yyjson_mut_arr(doc_));
    }

    template <typename T>
    auto create_object(const T& val) -> JsonObjectRef
    {
      return soul_op_build_json(this, val);
    }

    template <typename T>
      requires(!is_same_v<T, char>)
    auto create_array(Span<const T*> span) -> JsonArrayRef
    {
      auto array_ref = JsonArrayRef(yyjson_mut_arr(doc_));
      for (const T& val : span)
      {
        array_ref.append(soul_op_build_json(this, val));
      }
      return array_ref;
    }

    [[nodiscard]]
    auto create_root_empty_object() -> JsonObjectRef
    {
      auto ref = JsonObjectRef(doc_, yyjson_mut_obj(doc_));
      set_root(ref);
      return ref;
    }

    template <typename T>
    auto create_root_object(const T& val) -> JsonObjectRef
    {
      const auto ref = create_object(val);
      set_root(ref);
      return ref;
    }

    void set_root(JsonRef ref)
    {
      yyjson_mut_doc_set_root(doc_, ref.get_val());
    }

    auto dump() -> String
    {
      return String::From(yyjson_mut_write(doc_, 0, nullptr));
    }

  private:
    yyjson_mut_doc* doc_;
  };
} // namespace soul

template <typename T>
auto soul_op_construct_from_json(soul::JsonReadRef val_ref) -> T;

namespace soul
{
  template <typename T>
  auto from_json_string(StringView json_str) -> T
  {
    yyjson_doc* doc = yyjson_read(json_str.begin(), json_str.size(), 0);
    SOUL_ASSERT(0, doc != nullptr);
    yyjson_val* root = yyjson_doc_get_root(doc);
    T result         = soul_op_construct_from_json<T>(JsonReadRef(root));
    yyjson_doc_free(doc);
    return result;
  }

  template <typename T>
    requires(!is_same_v<T, char>)
  auto soul_op_build_json(JsonDoc* builder, Span<const T*> span) -> JsonRef
  {
    auto json_ref = builder->create_empty_array();
    for (const T& val : span)
    {
      json_ref.append(soul_op_build_json(builder, val));
    }
  }

} // namespace soul
