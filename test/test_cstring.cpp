#include <format>
#include <gtest/gtest.h>

#include "core/config.h"
#include "core/cstring.h"
#include "memory/allocator.h"

#include "util.h"

namespace soul
{
  auto get_default_allocator() -> memory::Allocator*
  {
    static TestAllocator test_allocator("Test default allocator");
    return &test_allocator;
  }
} // namespace soul

constexpr usize TEST_INLINE_CAPACITY = 32;
using TestString = soul::BasicCString<soul::memory::Allocator, TEST_INLINE_CAPACITY>;

constexpr const char* TEST_SHORT_STR = "abcdef";
constexpr const auto TEST_SHORT_STR_SIZE = soul::str_length(TEST_SHORT_STR);
static_assert(TEST_SHORT_STR_SIZE + 1 < TEST_INLINE_CAPACITY);

constexpr const char* TEST_SHORT_STR2 = "adefghbc";
constexpr const auto TEST_SHORT_STR_SIZE2 = soul::str_length(TEST_SHORT_STR2);
static_assert(TEST_SHORT_STR_SIZE2 + 1 < TEST_INLINE_CAPACITY);

constexpr const char* TEST_MAX_INLINE_STR = "abcdefghijklmnopqrstvuwxyz12345";
static_assert(soul::str_length(TEST_MAX_INLINE_STR) == (TEST_INLINE_CAPACITY - 1));

constexpr const char* TEST_MAX_INLINE_STR2 = "12345abcdefghijklmnopqrstvuwxyz";
static_assert(soul::str_length(TEST_MAX_INLINE_STR2) == (TEST_INLINE_CAPACITY - 1));

constexpr const char* TEST_LONG_STR = R"(
Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do 
eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut 
enim ad minim veniam, quis nostrud exercitation ullamco laboris 
nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in 
reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla 
pariatur. Excepteur sint occaecat cupidatat non proident, sunt in 
culpa qui officia deserunt mollit anim id est laborum.
)";
constexpr const auto TEST_LONG_STR_SIZE = soul::str_length(TEST_LONG_STR);
static_assert(TEST_LONG_STR_SIZE + 1 > TEST_INLINE_CAPACITY);

constexpr const char* TEST_LONG_STR2 = R"(
Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do 
eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut 
enim ad minim veniam, quis nostrud exercitation ullamco laboris 
nisi consequat. Duis aute irure dolor in 
reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla 
pariatur. Excepteur sint occaecat cupidatat non proident, sunt in 
culpa qui officia deserunt mollit anim id est laborum.
)";
constexpr const auto TEST_LONG_STR2_SIZE = soul::str_length(TEST_LONG_STR2);
static_assert(TEST_LONG_STR2_SIZE + 1 > TEST_INLINE_CAPACITY);

template <typename T>
concept cstring_source = soul::is_same_v<T, TestString> || std::is_same_v<T, const char*>;

template <cstring_source T>
auto construct_cstring(const T& src) -> TestString
{
  if constexpr (soul::is_same_v<T, TestString>) {
    return src.clone();
  } else {
    return TestString::From(src);
  }
}

void verify_equal(const TestString& result_str, const char* expected_str)
{
  SOUL_TEST_ASSERT_STREQ(result_str.data(), expected_str);
  SOUL_TEST_ASSERT_EQ(result_str.size(), strlen(expected_str));
}

void verify_equal(const TestString& result_str, const TestString& expected_str)
{
  SOUL_TEST_ASSERT_EQ(result_str, expected_str);
  SOUL_TEST_ASSERT_EQ(result_str.size(), expected_str.size());

  verify_equal(result_str, expected_str.data());
  SOUL_TEST_ASSERT_STREQ(result_str.data(), expected_str.data());
}

#include "common_test.h"

TEST(TestCStringConstruction, TestDefaultConstructor)
{
  const TestString cstring;
  verify_equal(cstring, "");
}

TEST(TestCStringConstruction, TestConstructionUnsharedFromCharArray)
{
  const auto test_construction_unshared_from = [](const char* str) {
    const auto test_string = TestString::UnsharedFrom(str);
    SOUL_TEST_RUN(verify_equal(test_string, str));
  };
  SOUL_TEST_RUN(test_construction_unshared_from(""));
  SOUL_TEST_RUN(test_construction_unshared_from(TEST_SHORT_STR));
  SOUL_TEST_RUN(test_construction_unshared_from(TEST_MAX_INLINE_STR));
  SOUL_TEST_RUN(test_construction_unshared_from(TEST_LONG_STR));
}

TEST(TestCStringConstruction, TestConstructFromCharArray)
{
  const auto test_construction_from = [](const char* str) {
    const auto cstring = TestString::From(str);
    SOUL_TEST_RUN(verify_equal(cstring, str));
  };

  SOUL_TEST_RUN(test_construction_from(""));
  SOUL_TEST_RUN(test_construction_from(TEST_SHORT_STR));
  SOUL_TEST_RUN(test_construction_from(TEST_MAX_INLINE_STR));
  SOUL_TEST_RUN(test_construction_from(TEST_LONG_STR));
}

TEST(TestCStringConstruction, TestConstructionWithSize)
{
  const auto test_construction_with_size = [](usize size) {
    const auto test_string = TestString::WithSize(size);
    SOUL_TEST_ASSERT_EQ(test_string.size(), size);
  };
  SOUL_TEST_RUN(test_construction_with_size(0));
  SOUL_TEST_RUN(test_construction_with_size(TEST_SHORT_STR_SIZE));
  SOUL_TEST_RUN(test_construction_with_size(TEST_INLINE_CAPACITY - 1));
  SOUL_TEST_RUN(test_construction_with_size(TEST_LONG_STR_SIZE));
}

TEST(TestCStringConstruction, TestConstructionFormat)
{
  SOUL_TEST_RUN(verify_equal(TestString::Format("{}", ""), ""));
  SOUL_TEST_RUN(verify_equal(TestString::Format("ab{}ef", "cd"), "abcdef"));
  SOUL_TEST_RUN(verify_equal(
    TestString::Format("abcdefghijkl{}rstuvwxyz12345", "mnopq"),
    "abcdefghijklmnopqrstuvwxyz12345"));
  SOUL_TEST_RUN(verify_equal(
    TestString::Format("abcdefghijkl{}rstuvwxyz1{}45", "mnopq", "23"),
    "abcdefghijklmnopqrstuvwxyz12345"));
  SOUL_TEST_RUN(verify_equal(
    TestString::Format("abcdefghijkl{}rstuvwxyz1{}4567890", "mnopq", "23"),
    "abcdefghijklmnopqrstuvwxyz1234567890"));
}

TEST(TestCStringConstruction, TestConstructionReservedFormat)
{
  SOUL_TEST_RUN(
    verify_equal(TestString::ReservedFormat(soul::get_default_allocator(), "{}", ""), ""));
  SOUL_TEST_RUN(verify_equal(
    TestString::ReservedFormat(soul::get_default_allocator(), "ab{}ef", "cd"), "abcdef"));
  SOUL_TEST_RUN(verify_equal(
    TestString::ReservedFormat(
      soul::get_default_allocator(), "abcdefghijkl{}rstuvwxyz12345", "mnopq"),
    "abcdefghijklmnopqrstuvwxyz12345"));
  SOUL_TEST_RUN(verify_equal(
    TestString::ReservedFormat(
      soul::get_default_allocator(), "abcdefghijkl{}rstuvwxyz1{}45", "mnopq", "23"),
    "abcdefghijklmnopqrstuvwxyz12345"));
  SOUL_TEST_RUN(verify_equal(
    TestString::ReservedFormat(
      soul::get_default_allocator(), "abcdefghijkl{}rstuvwxyz1{}4567890", "mnopq", "23"),
    "abcdefghijklmnopqrstuvwxyz1234567890"));
}

TEST(TestCStringConstruction, TestConstructionWithCapacity)
{
  const auto test_construction_with_capacity = [](usize capacity) {
    const auto test_string = TestString::WithCapacity(capacity);
    SOUL_TEST_ASSERT_GE(test_string.capacity(), capacity);
  };
  SOUL_TEST_RUN(test_construction_with_capacity(0));
  SOUL_TEST_RUN(test_construction_with_capacity(TEST_SHORT_STR_SIZE));
  SOUL_TEST_RUN(test_construction_with_capacity(TEST_INLINE_CAPACITY - 1));
  SOUL_TEST_RUN(test_construction_with_capacity(TEST_LONG_STR_SIZE));
}

TEST(TestCStringConstruction, TestCustomAllocatorDefaultConstructor)
{
  TestAllocator::reset_all();
  TestAllocator test_allocator;
  TestString cstring(&test_allocator);
  SOUL_TEST_ASSERT_STREQ(cstring.data(), "");
  SOUL_TEST_ASSERT_EQ(cstring.size(), 0);

  const auto post_reserve_alloc_count = test_allocator.allocCount;
  cstring.reserve(10);
  SOUL_TEST_ASSERT_GE(cstring.capacity(), 10);
}

TEST(TestCStringConstruction, TestClone)
{
  SOUL_TEST_RUN(test_clone(TestString::From(TEST_SHORT_STR)));
  SOUL_TEST_RUN(test_clone(TestString::UnsharedFrom(TEST_SHORT_STR)));
  SOUL_TEST_RUN(test_clone(TestString::From(TEST_MAX_INLINE_STR)));
  SOUL_TEST_RUN(test_clone(TestString::UnsharedFrom(TEST_MAX_INLINE_STR)));
  SOUL_TEST_RUN(test_clone(TestString::From(TEST_LONG_STR)));
  SOUL_TEST_RUN(test_clone(TestString::UnsharedFrom(TEST_LONG_STR)));
  SOUL_TEST_RUN(test_clone(TestString::From("")));
  SOUL_TEST_RUN(test_clone(TestString::UnsharedFrom("")));
}

TEST(TestCStringConstruction, TestMoveConstructor)
{
  SOUL_TEST_RUN(test_move_constructor(TestString::From(TEST_SHORT_STR)));
  SOUL_TEST_RUN(test_move_constructor(TestString::UnsharedFrom(TEST_SHORT_STR)));
  SOUL_TEST_RUN(test_move_constructor(TestString::From(TEST_MAX_INLINE_STR)));
  SOUL_TEST_RUN(test_move_constructor(TestString::UnsharedFrom(TEST_MAX_INLINE_STR)));
  SOUL_TEST_RUN(test_move_constructor(TestString::From(TEST_LONG_STR)));
  SOUL_TEST_RUN(test_move_constructor(TestString::UnsharedFrom(TEST_LONG_STR)));
  SOUL_TEST_RUN(test_move_constructor(TestString::From("")));
  SOUL_TEST_RUN(test_move_constructor(TestString::UnsharedFrom("")));
}

class TestCStringManipulation : public testing::Test
{
public:
  TestString test_const_segment_string = TestString::From(TEST_SHORT_STR);
  TestString test_const_segment_string2 = TestString::From(TEST_LONG_STR);

  TestString test_short_string = TestString::UnsharedFrom(TEST_SHORT_STR);
  TestString test_short_string2 = TestString::UnsharedFrom(TEST_SHORT_STR2);

  TestString test_max_inline_string = TestString::UnsharedFrom(TEST_MAX_INLINE_STR);
  TestString test_max_inline_string2 = TestString::UnsharedFrom(TEST_MAX_INLINE_STR2);

  TestString test_long_string = TestString::UnsharedFrom(TEST_LONG_STR);
  TestString test_long_string2 = TestString::UnsharedFrom(TEST_LONG_STR2);
};

TEST_F(TestCStringManipulation, TestMoveAssignemnt)
{
  SOUL_TEST_RUN(test_move_assignment(test_const_segment_string, test_const_segment_string2));
  SOUL_TEST_RUN(test_move_assignment(test_const_segment_string, test_short_string2));
  SOUL_TEST_RUN(test_move_assignment(test_const_segment_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_move_assignment(test_const_segment_string, test_long_string2));
  SOUL_TEST_RUN(test_move_assignment(test_const_segment_string, TestString()));

  SOUL_TEST_RUN(test_move_assignment(test_short_string, test_const_segment_string2));
  SOUL_TEST_RUN(test_move_assignment(test_short_string, test_short_string2));
  SOUL_TEST_RUN(test_move_assignment(test_short_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_move_assignment(test_short_string, test_long_string2));
  SOUL_TEST_RUN(test_move_assignment(test_short_string, TestString()));

  SOUL_TEST_RUN(test_move_assignment(test_max_inline_string, test_const_segment_string2));
  SOUL_TEST_RUN(test_move_assignment(test_max_inline_string, test_short_string2));
  SOUL_TEST_RUN(test_move_assignment(test_max_inline_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_move_assignment(test_max_inline_string, test_long_string2));
  SOUL_TEST_RUN(test_move_assignment(test_max_inline_string, TestString()));

  SOUL_TEST_RUN(test_move_assignment(test_long_string, test_const_segment_string2));
  SOUL_TEST_RUN(test_move_assignment(test_long_string, test_short_string2));
  SOUL_TEST_RUN(test_move_assignment(test_long_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_move_assignment(test_long_string, test_long_string2));
  SOUL_TEST_RUN(test_move_assignment(test_long_string, TestString()));

  SOUL_TEST_RUN(test_move_assignment(TestString(), test_const_segment_string2));
  SOUL_TEST_RUN(test_move_assignment(TestString(), test_short_string2));
  SOUL_TEST_RUN(test_move_assignment(TestString(), test_max_inline_string2));
  SOUL_TEST_RUN(test_move_assignment(TestString(), test_long_string2));
  SOUL_TEST_RUN(test_move_assignment(TestString(), TestString()));
}

TEST_F(TestCStringManipulation, TestCloneFrom)
{
  SOUL_TEST_RUN(test_clone_from(test_const_segment_string, test_const_segment_string2));
  SOUL_TEST_RUN(test_clone_from(test_const_segment_string, test_short_string2));
  SOUL_TEST_RUN(test_clone_from(test_const_segment_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_clone_from(test_const_segment_string, test_long_string2));
  SOUL_TEST_RUN(test_clone_from(test_const_segment_string, TestString()));

  SOUL_TEST_RUN(test_clone_from(test_short_string, test_const_segment_string2));
  SOUL_TEST_RUN(test_clone_from(test_short_string, test_short_string2));
  SOUL_TEST_RUN(test_clone_from(test_short_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_clone_from(test_short_string, test_long_string2));
  SOUL_TEST_RUN(test_clone_from(test_short_string, TestString()));

  SOUL_TEST_RUN(test_clone_from(test_max_inline_string, test_const_segment_string2));
  SOUL_TEST_RUN(test_clone_from(test_max_inline_string, test_short_string2));
  SOUL_TEST_RUN(test_clone_from(test_max_inline_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_clone_from(test_max_inline_string, test_long_string2));
  SOUL_TEST_RUN(test_clone_from(test_max_inline_string, TestString()));

  SOUL_TEST_RUN(test_clone_from(test_long_string, test_const_segment_string2));
  SOUL_TEST_RUN(test_clone_from(test_long_string, test_short_string2));
  SOUL_TEST_RUN(test_clone_from(test_long_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_clone_from(test_long_string, test_long_string2));
  SOUL_TEST_RUN(test_clone_from(test_long_string, TestString()));

  SOUL_TEST_RUN(test_clone_from(TestString(), test_const_segment_string2));
  SOUL_TEST_RUN(test_clone_from(TestString(), test_short_string2));
  SOUL_TEST_RUN(test_clone_from(TestString(), test_max_inline_string2));
  SOUL_TEST_RUN(test_clone_from(TestString(), test_long_string2));
  SOUL_TEST_RUN(test_clone_from(TestString(), TestString()));
}

TEST_F(TestCStringManipulation, TestCStringSwap)
{
  SOUL_TEST_RUN(test_swap(test_const_segment_string, test_const_segment_string2));
  SOUL_TEST_RUN(test_swap(test_const_segment_string, test_short_string2));
  SOUL_TEST_RUN(test_swap(test_const_segment_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_swap(test_const_segment_string, test_long_string2));
  SOUL_TEST_RUN(test_swap(test_const_segment_string, TestString()));

  SOUL_TEST_RUN(test_swap(test_short_string, test_const_segment_string2));
  SOUL_TEST_RUN(test_swap(test_short_string, test_short_string2));
  SOUL_TEST_RUN(test_swap(test_short_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_swap(test_short_string, test_long_string2));
  SOUL_TEST_RUN(test_swap(test_short_string, TestString()));

  SOUL_TEST_RUN(test_swap(test_max_inline_string, test_const_segment_string2));
  SOUL_TEST_RUN(test_swap(test_max_inline_string, test_short_string2));
  SOUL_TEST_RUN(test_swap(test_max_inline_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_swap(test_max_inline_string, test_long_string2));
  SOUL_TEST_RUN(test_swap(test_max_inline_string, TestString()));

  SOUL_TEST_RUN(test_swap(test_long_string, test_const_segment_string2));
  SOUL_TEST_RUN(test_swap(test_long_string, test_short_string2));
  SOUL_TEST_RUN(test_swap(test_long_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_swap(test_long_string, test_long_string2));
  SOUL_TEST_RUN(test_swap(test_long_string, TestString()));

  SOUL_TEST_RUN(test_swap(TestString(), test_const_segment_string2));
  SOUL_TEST_RUN(test_swap(TestString(), test_short_string2));
  SOUL_TEST_RUN(test_swap(TestString(), test_max_inline_string2));
  SOUL_TEST_RUN(test_swap(TestString(), test_long_string2));
  SOUL_TEST_RUN(test_swap(TestString(), TestString()));
}

TEST_F(TestCStringManipulation, TestReserve)
{
  const auto test_reserve = [](const TestString& string_src, usize new_capacity) {
    auto test_string = string_src.clone();
    test_string.reserve(new_capacity);
    SOUL_TEST_ASSERT_GE(test_string.capacity(), new_capacity);
    verify_equal(test_string, string_src);
  };

  SOUL_TEST_RUN(test_reserve(test_const_segment_string, 0));
  SOUL_TEST_RUN(test_reserve(test_const_segment_string, TEST_SHORT_STR_SIZE));
  SOUL_TEST_RUN(test_reserve(test_const_segment_string, TEST_INLINE_CAPACITY));
  SOUL_TEST_RUN(test_reserve(test_const_segment_string, TEST_LONG_STR_SIZE));

  SOUL_TEST_RUN(test_reserve(test_short_string, 0));
  SOUL_TEST_RUN(test_reserve(test_short_string, TEST_SHORT_STR_SIZE));
  SOUL_TEST_RUN(test_reserve(test_short_string, TEST_INLINE_CAPACITY));
  SOUL_TEST_RUN(test_reserve(test_short_string, TEST_LONG_STR_SIZE));

  SOUL_TEST_RUN(test_reserve(test_max_inline_string, 0));
  SOUL_TEST_RUN(test_reserve(test_max_inline_string, TEST_SHORT_STR_SIZE));
  SOUL_TEST_RUN(test_reserve(test_max_inline_string, TEST_INLINE_CAPACITY));
  SOUL_TEST_RUN(test_reserve(test_max_inline_string, TEST_LONG_STR_SIZE));

  SOUL_TEST_RUN(test_reserve(test_long_string, 0));
  SOUL_TEST_RUN(test_reserve(test_long_string, TEST_SHORT_STR_SIZE));
  SOUL_TEST_RUN(test_reserve(test_long_string, TEST_INLINE_CAPACITY));
  SOUL_TEST_RUN(test_reserve(test_long_string, TEST_LONG_STR_SIZE));

  SOUL_TEST_RUN(test_reserve(TestString(), 0));
  SOUL_TEST_RUN(test_reserve(TestString(), TEST_SHORT_STR_SIZE));
  SOUL_TEST_RUN(test_reserve(TestString(), TEST_INLINE_CAPACITY));
  SOUL_TEST_RUN(test_reserve(TestString(), TEST_LONG_STR_SIZE));
}

TEST_F(TestCStringManipulation, TestClear)
{
  const auto test_clear = [](const TestString& sample_string) {
    auto test_string = sample_string.clone();
    test_string.clear();
    verify_equal(test_string, "");
  };

  SOUL_TEST_RUN(test_clear(test_const_segment_string));
  SOUL_TEST_RUN(test_clear(test_short_string));
  SOUL_TEST_RUN(test_clear(test_max_inline_string));
  SOUL_TEST_RUN(test_clear(test_long_string));
  SOUL_TEST_RUN(test_clear(TestString()));
}

TEST_F(TestCStringManipulation, TestPushBack)
{
  const auto test_push_back = [](const TestString& sample_string, char c) {
    auto test_string = sample_string.clone();
    std::string expected_string(sample_string.data());
    test_string.push_back(c);
    expected_string.push_back(c);
    verify_equal(test_string, expected_string.data());
  };

  SOUL_TEST_RUN(test_push_back(test_short_string, 'x'));
  SOUL_TEST_RUN(test_push_back(test_max_inline_string, 'x'));
  SOUL_TEST_RUN(test_push_back(test_long_string, 'x'));
  SOUL_TEST_RUN(test_push_back(TestString(), 'x'));
}

TEST_F(TestCStringManipulation, TestAppendCharArr)
{
  const auto test_append = [](const TestString& sample_string, const char* extra_str) {
    auto test_string = sample_string.clone();
    std::string expected_string(sample_string.data());
    test_string.append(extra_str);
    expected_string += std::string(extra_str);
    verify_equal(test_string, expected_string.data());
  };

  SOUL_TEST_RUN(test_append(test_const_segment_string, TEST_SHORT_STR));
  SOUL_TEST_RUN(test_append(test_const_segment_string, TEST_MAX_INLINE_STR));
  SOUL_TEST_RUN(test_append(test_const_segment_string, TEST_LONG_STR));
  SOUL_TEST_RUN(test_append(test_const_segment_string, ""));

  SOUL_TEST_RUN(test_append(test_short_string, TEST_SHORT_STR));
  SOUL_TEST_RUN(test_append(test_short_string, TEST_MAX_INLINE_STR));
  SOUL_TEST_RUN(test_append(test_short_string, TEST_LONG_STR));
  SOUL_TEST_RUN(test_append(test_short_string, ""));

  SOUL_TEST_RUN(test_append(test_max_inline_string, TEST_SHORT_STR));
  SOUL_TEST_RUN(test_append(test_max_inline_string, TEST_MAX_INLINE_STR));
  SOUL_TEST_RUN(test_append(test_max_inline_string, TEST_LONG_STR));
  SOUL_TEST_RUN(test_append(test_max_inline_string, ""));

  SOUL_TEST_RUN(test_append(test_long_string, TEST_SHORT_STR));
  SOUL_TEST_RUN(test_append(test_long_string, TEST_MAX_INLINE_STR));
  SOUL_TEST_RUN(test_append(test_long_string, TEST_LONG_STR));
  SOUL_TEST_RUN(test_append(test_long_string, ""));

  SOUL_TEST_RUN(test_append(TestString(), TEST_SHORT_STR));
  SOUL_TEST_RUN(test_append(TestString(), TEST_MAX_INLINE_STR));
  SOUL_TEST_RUN(test_append(TestString(), TEST_LONG_STR));
  SOUL_TEST_RUN(test_append(TestString(), ""));
}

TEST_F(TestCStringManipulation, TestAppend)
{
  const auto test_append = [](const TestString& sample_string, const TestString& extra_string) {
    auto test_string = sample_string.clone();
    std::string expected_string(sample_string.data());
    test_string.append(extra_string.data());
    expected_string += std::string(extra_string.data());
    verify_equal(test_string, expected_string.data());
  };

  SOUL_TEST_RUN(test_append(test_const_segment_string, test_short_string2));
  SOUL_TEST_RUN(test_append(test_const_segment_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_append(test_const_segment_string, test_long_string2));
  SOUL_TEST_RUN(test_append(test_const_segment_string, TestString()));

  SOUL_TEST_RUN(test_append(test_short_string, test_short_string2));
  SOUL_TEST_RUN(test_append(test_short_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_append(test_short_string, test_long_string2));
  SOUL_TEST_RUN(test_append(test_short_string, TestString()));

  SOUL_TEST_RUN(test_append(test_max_inline_string, test_short_string2));
  SOUL_TEST_RUN(test_append(test_max_inline_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_append(test_max_inline_string, test_long_string2));
  SOUL_TEST_RUN(test_append(test_max_inline_string, TestString()));

  SOUL_TEST_RUN(test_append(test_long_string, test_short_string2));
  SOUL_TEST_RUN(test_append(test_long_string, test_max_inline_string2));
  SOUL_TEST_RUN(test_append(test_long_string, test_long_string2));
  SOUL_TEST_RUN(test_append(test_long_string, TestString()));

  SOUL_TEST_RUN(test_append(TestString(), test_short_string2));
  SOUL_TEST_RUN(test_append(TestString(), test_max_inline_string2));
  SOUL_TEST_RUN(test_append(TestString(), test_long_string2));
  SOUL_TEST_RUN(test_append(TestString(), TestString()));
}

TEST_F(TestCStringManipulation, TestAppendFormat)
{
  const auto test_append_format =
    []<typename... Args>(
      const TestString& sample_string, std::format_string<Args...> fmt, Args&&... args) {
      auto test_string = sample_string.clone();
      const auto expected_string =
        std::string(sample_string.data()) + std::vformat(fmt.get(), std::make_format_args(args...));
      test_string.appendf(std::move(fmt), std::forward<Args>(args)...);
      verify_equal(test_string, expected_string.data());
    };

  SOUL_TEST_RUN(test_append_format(test_const_segment_string, "ab{}ef", "cd"));
  SOUL_TEST_RUN(
    test_append_format(test_const_segment_string, "abcdefghijkl{}rstuvwxyz1{}45", "mnopq", "23"));

  SOUL_TEST_RUN(test_append_format(test_short_string, "{}", TEST_SHORT_STR2));
  SOUL_TEST_RUN(test_append_format(test_short_string, "{}", TEST_MAX_INLINE_STR2));
  SOUL_TEST_RUN(test_append_format(test_short_string, "{}", TEST_LONG_STR2));
  SOUL_TEST_RUN(test_append_format(test_short_string, "{}", ""));

  SOUL_TEST_RUN(test_append_format(test_max_inline_string, "{}", TEST_SHORT_STR2));
  SOUL_TEST_RUN(test_append_format(test_max_inline_string, "{}", TEST_MAX_INLINE_STR2));
  SOUL_TEST_RUN(test_append_format(test_max_inline_string, "{}", TEST_LONG_STR2));
  SOUL_TEST_RUN(test_append_format(test_max_inline_string, "{}", ""));

  SOUL_TEST_RUN(test_append_format(test_long_string, "{}", TEST_SHORT_STR2));
  SOUL_TEST_RUN(test_append_format(test_long_string, "{}", TEST_MAX_INLINE_STR2));
  SOUL_TEST_RUN(test_append_format(test_long_string, "{}", TEST_LONG_STR2));
  SOUL_TEST_RUN(test_append_format(test_long_string, "{}", ""));
}

TEST_F(TestCStringManipulation, TestAssign)
{
  const auto test_assign = [](const TestString& sample_string, const char* assigned_str) {
    auto test_string = sample_string.clone();
    test_string.assign(assigned_str);
    verify_equal(test_string, assigned_str);
  };

  SOUL_TEST_RUN(test_assign(test_const_segment_string, TEST_SHORT_STR));
  SOUL_TEST_RUN(test_assign(test_const_segment_string, TEST_MAX_INLINE_STR));
  SOUL_TEST_RUN(test_assign(test_const_segment_string, TEST_LONG_STR));
  SOUL_TEST_RUN(test_assign(test_const_segment_string, ""));

  SOUL_TEST_RUN(test_assign(test_short_string, TEST_SHORT_STR));
  SOUL_TEST_RUN(test_assign(test_short_string, TEST_MAX_INLINE_STR));
  SOUL_TEST_RUN(test_assign(test_short_string, TEST_LONG_STR));
  SOUL_TEST_RUN(test_assign(test_short_string, ""));

  SOUL_TEST_RUN(test_assign(test_max_inline_string, TEST_SHORT_STR));
  SOUL_TEST_RUN(test_assign(test_max_inline_string, TEST_MAX_INLINE_STR));
  SOUL_TEST_RUN(test_assign(test_max_inline_string, TEST_LONG_STR));
  SOUL_TEST_RUN(test_assign(test_max_inline_string, ""));

  SOUL_TEST_RUN(test_assign(test_long_string, TEST_SHORT_STR));
  SOUL_TEST_RUN(test_assign(test_long_string, TEST_MAX_INLINE_STR));
  SOUL_TEST_RUN(test_assign(test_long_string, TEST_LONG_STR));
  SOUL_TEST_RUN(test_assign(test_long_string, ""));

  SOUL_TEST_RUN(test_assign(TestString(), TEST_SHORT_STR));
  SOUL_TEST_RUN(test_assign(TestString(), TEST_MAX_INLINE_STR));
  SOUL_TEST_RUN(test_assign(TestString(), TEST_LONG_STR));
  SOUL_TEST_RUN(test_assign(TestString(), ""));
}

TEST_F(TestCStringManipulation, TestAssignFormat)
{
  const auto test_assign_format =
    []<typename... Args>(
      const TestString& sample_string, std::format_string<Args...> fmt, Args&&... args) {
      auto test_string = sample_string.clone();
      const auto expected_string = std::vformat(fmt.get(), std::make_format_args(args...));
      test_string.assignf(std::move(fmt), std::forward<Args>(args)...);
      verify_equal(test_string, expected_string.data());
    };

  SOUL_TEST_RUN(test_assign_format(test_const_segment_string, "ab{}ef", "cd"));
  SOUL_TEST_RUN(
    test_assign_format(test_const_segment_string, "abcdefghijkl{}rstuvwxyz1{}45", "mnopq", "23"));

  SOUL_TEST_RUN(test_assign_format(test_short_string, "{}", TEST_SHORT_STR2));
  SOUL_TEST_RUN(test_assign_format(test_short_string, "{}", TEST_MAX_INLINE_STR2));
  SOUL_TEST_RUN(test_assign_format(test_short_string, "{}", TEST_LONG_STR2));
  SOUL_TEST_RUN(test_assign_format(test_short_string, "{}", ""));

  SOUL_TEST_RUN(test_assign_format(test_max_inline_string, "{}", TEST_SHORT_STR2));
  SOUL_TEST_RUN(test_assign_format(test_max_inline_string, "{}", TEST_MAX_INLINE_STR2));
  SOUL_TEST_RUN(test_assign_format(test_max_inline_string, "{}", TEST_LONG_STR2));
  SOUL_TEST_RUN(test_assign_format(test_max_inline_string, "{}", ""));

  SOUL_TEST_RUN(test_assign_format(test_long_string, "{}", TEST_SHORT_STR2));
  SOUL_TEST_RUN(test_assign_format(test_long_string, "{}", TEST_MAX_INLINE_STR2));
  SOUL_TEST_RUN(test_assign_format(test_long_string, "{}", TEST_LONG_STR2));
  SOUL_TEST_RUN(test_assign_format(test_long_string, "{}", ""));
}

TEST(TestCStringFormat, TestCStringFormat)
{
  SOUL_TEST_RUN(verify_equal(TestString::Format("{}", TestString()), ""));
  SOUL_TEST_RUN(verify_equal(TestString::Format("{}", TestString::From(TEST_SHORT_STR)), "abcdef"));
  SOUL_TEST_RUN(verify_equal(
    TestString::Format("{}", TestString::From(TEST_MAX_INLINE_STR)), TEST_MAX_INLINE_STR));
  SOUL_TEST_RUN(
    verify_equal(TestString::Format("{}", TestString::From(TEST_LONG_STR)), TEST_LONG_STR));
};

TEST(TestCStringHash, TestCStringHash)
{
  std::hash<TestString> hasher;

  TestString test_const_segment_string = TestString::From(TEST_SHORT_STR);
  TestString test_const_segment_string2 = TestString::From(TEST_LONG_STR);

  TestString test_short_string = TestString::UnsharedFrom(TEST_SHORT_STR);
  TestString test_short_string2 = TestString::UnsharedFrom(TEST_SHORT_STR2);

  TestString test_max_inline_string = TestString::UnsharedFrom(TEST_MAX_INLINE_STR);
  TestString test_max_inline_string2 = TestString::UnsharedFrom(TEST_MAX_INLINE_STR2);

  TestString test_long_string = TestString::UnsharedFrom(TEST_LONG_STR);
  TestString test_long_string2 = TestString::UnsharedFrom(TEST_LONG_STR2);

  SOUL_TEST_ASSERT_EQ(hasher(test_const_segment_string), hasher(test_const_segment_string));
  SOUL_TEST_ASSERT_NE(hasher(test_const_segment_string), hasher(test_const_segment_string2));
  SOUL_TEST_ASSERT_EQ(hasher(test_const_segment_string), hasher(test_short_string));

  SOUL_TEST_ASSERT_EQ(hasher(test_short_string), hasher(test_short_string));
  SOUL_TEST_ASSERT_NE(hasher(test_short_string), hasher(test_short_string2));
  SOUL_TEST_ASSERT_NE(hasher(test_short_string), hasher(test_max_inline_string));
  SOUL_TEST_ASSERT_NE(hasher(test_short_string), hasher(test_long_string));

  SOUL_TEST_ASSERT_EQ(hasher(test_max_inline_string), hasher(test_max_inline_string));
  SOUL_TEST_ASSERT_NE(hasher(test_max_inline_string), hasher(test_max_inline_string2));
  SOUL_TEST_ASSERT_NE(hasher(test_max_inline_string), hasher(test_long_string));

  SOUL_TEST_ASSERT_EQ(hasher(test_long_string), hasher(test_long_string));
  SOUL_TEST_ASSERT_NE(hasher(test_long_string), hasher(test_long_string2));
}
