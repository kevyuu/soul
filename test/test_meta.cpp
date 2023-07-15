#include <gtest/gtest.h>

#include "core/config.h"
#include "core/meta.h"
#include "core/objops.h"
#include "core/option.h"
#include "core/type_traits.h"
#include "core/views.h"
#include "memory/allocator.h"

#include "util.h"

static_assert(soul::get_type_index_v<int, int, b8, uint8_t, uint16_t, char> == 0);

static_assert(soul::get_type_index_v<uint8_t, int, b8, uint8_t, uint16_t, char> == 2);

static_assert(soul::get_type_index_v<char, int, b8, uint8_t, uint16_t, char> == 4);

static_assert(soul::is_same_v<soul::get_type_at_t<0, int, b8, uint8_t, uint16_t, char>, int>);

static_assert(soul::is_same_v<soul::get_type_at_t<2, int, b8, uint8_t, uint16_t, char>, uint8_t>);

static_assert(soul::is_same_v<soul::get_type_at_t<4, int, b8, uint8_t, uint16_t, char>, char>);

static_assert(soul::get_type_count_v<int, int, b8, char> == 1);

static_assert(soul::get_type_count_v<int, b8, char, uint8_t> == 0);

static_assert(soul::get_type_count_v<int, int, int, int, int, int> == 5);

static_assert(!soul::has_duplicate_type_v<int>);

static_assert(soul::has_duplicate_type_v<int, int>);

static_assert(!soul::has_duplicate_type_v<int, b8, char, uint8_t>);
