#include <gtest/gtest.h>

#include "common_test.h"

#include "core/array.h"
#include "core/config.h"
#include "core/string.h"
#include "core/hash.h"
#include "core/type_traits.h"
#include "memory/allocator.h"

#include "util.h"
#include <type_traits>

TEST(TestHash, TestHashIntegral)
{
  SOUL_TEST_RUN(test_hash_implementation(soul::Array{1, 3, 8, 16, 10000000, 1200, 1024}));
}

TEST(TestHash, TestHashBool) { SOUL_TEST_ASSERT_NE(soul::hash(false), soul::hash(true)); }

TEST(TestHash, TestHashFloatingPoint)
{
  SOUL_TEST_RUN(test_hash_implementation(soul::Array{
    0.0f,
    1.0f,
    1.2f,
    3.14f,
  }));

  SOUL_TEST_RUN(test_hash_implementation(soul::Array{
    0.0,
    1.0,
    3.14,
    281314585773.3209,
  }));
}

TEST(TestHah, TestHashScopedEnum)
{
  enum TestEnum2 { ONE, TWO };

  enum class TestEnum : u32 {
    ONE,
    TWO,
    THREE,
    FOUR,
    COUNT,
  };

  SOUL_TEST_RUN(test_hash_implementation(soul::Array{
    TestEnum::ONE,
    TestEnum::TWO,
    TestEnum::THREE,
    TestEnum::FOUR,
    TestEnum::COUNT,
  }));
}

TEST(TestHash, TestHashBytes)
{
  auto span_from_chars = [](const char* chars) -> soul::Span<const char*> {
    return {chars, strlen(chars)};
  };

  using namespace soul;
  SOUL_TEST_RUN(test_hash_span_implementation(soul::Array{
    span_from_chars("test1"),
    span_from_chars("test2"),
    span_from_chars("test3"),
    span_from_chars("long_string_test"),
  }));
}

struct TestCombineObj {
  u32 x;
  u64 y;
};
constexpr void soul_op_hash_combine(auto& hasher, const TestCombineObj& val)
{
  hasher.combine(val.x);
  hasher.combine(val.y);
}
static_assert(soul::impl_soul_op_hash_combine_v<TestCombineObj>);

struct TestCombineObj2 {
  u32 x;
  u64 y;
};
constexpr void soul_op_hash_combine(auto& hasher, const TestCombineObj2& val)
{
  hasher.combine(val.x);
}
static_assert(soul::impl_soul_op_hash_combine_v<TestCombineObj2>);

TEST(TestHash, TestCustomCombine)
{
  {
    test_hash_implementation(soul::Array{
      TestCombineObj{1, 3},
      TestCombineObj{1, 2},
      TestCombineObj{3, 1},
    });
  }

  {
    SOUL_TEST_ASSERT_EQ(soul::hash(TestCombineObj2{1, 2}), soul::hash(TestCombineObj2{1, 3}));

    test_hash_implementation(soul::Array{
      TestCombineObj2{1, 3},
      TestCombineObj2{4, 1},
      TestCombineObj2{3, 1},
    });
  }
}
