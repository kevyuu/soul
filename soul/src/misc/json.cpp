#include "misc/json.h"
#include <yyjson.h>
#include "core/integer.h"
#include "core/string_view.h"

namespace soul
{
  JsonReadRef::JsonReadRef(yyjson_val* val_ptr) : val_ptr_(val_ptr) {}

  auto JsonReadRef::get_type() const -> JsonType
  {
    if (val_ptr_ == nullptr)
    {
      return JsonType::NONE;
    }
    switch (yyjson_get_type(val_ptr_))
    {
    case YYJSON_TYPE_NONE: return JsonType::NONE;
    case YYJSON_TYPE_NULL: return JsonType::NIL;
    case YYJSON_TYPE_BOOL: return JsonType::BOOL;
    case YYJSON_TYPE_NUM: return JsonType::NUMBER;
    case YYJSON_TYPE_STR: return JsonType::STRING;
    case YYJSON_TYPE_ARR: return JsonType::ARRAY;
    case YYJSON_TYPE_OBJ: return JsonType::OBJECT;
    }
    unreachable();
    return JsonType::COUNT;
  }

  auto JsonReadRef::ref(StringView key) const -> JsonReadRef
  {
    return JsonReadRef(yyjson_obj_getn(val_ptr_, key.begin(), key.size()));
  }

  auto JsonReadRef::as_raw() const -> StringView
  {
    return StringView(yyjson_get_raw(val_ptr_), yyjson_get_len(val_ptr_));
  }

  auto JsonReadRef::as_string_view() const -> StringView
  {
    return StringView(yyjson_get_str(val_ptr_), yyjson_get_len(val_ptr_));
  }

  auto JsonReadRef::as_i32() const -> i32
  {
    return yyjson_get_int(val_ptr_);
  }

  auto JsonReadRef::as_i64() const -> i64
  {
    return yyjson_get_sint(val_ptr_);
  }

  auto JsonReadRef::as_u32() const -> u32
  {
    return cast<u32>(yyjson_get_uint(val_ptr_));
  }

  auto JsonReadRef::as_u64() const -> u64
  {
    return yyjson_get_uint(val_ptr_);
  }

  auto JsonReadRef::as_f32() const -> f32
  {
    return static_cast<f32>(yyjson_get_real(val_ptr_));
  }

  auto JsonReadRef::as_f64() const -> f64
  {
    return yyjson_get_real(val_ptr_);
  }

  auto JsonReadRef::as_b8() const -> b8
  {
    return yyjson_get_bool(val_ptr_);
  }

  auto JsonReadRef::as_string_view_or(StringView default_val) const -> StringView
  {
    if (get_type() != JsonType::STRING)
    {
      return default_val;
    }
    return as_string_view();
  }

  auto JsonReadRef::as_i32_or(i32 default_val) const -> i32
  {
    if (get_type() != JsonType::NUMBER)
    {
      return default_val;
    }
    return as_i32();
  }

  auto JsonReadRef::as_i64_or(i64 default_val) const -> i64
  {
    if (get_type() != JsonType::NUMBER)
    {
      return default_val;
    }
    return as_i64();
  }

  auto JsonReadRef::as_u64_or(u64 default_val) const -> u64
  {
    if (get_type() != JsonType::NUMBER)
    {
      return default_val;
    }
    return as_u64();
  }

  auto JsonReadRef::as_f64_or(f64 default_val) const -> f64
  {
    if (get_type() != JsonType::NUMBER)
    {
      return default_val;
    }
    return as_f64();
  }

  auto JsonReadRef::as_b8_or(b8 default_val) const -> b8
  {
    if (get_type() != JsonType::BOOL)
    {
      return default_val;
    }
    return as_b8();
  }
} // namespace soul
