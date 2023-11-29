#include <format>
#include <gtest/gtest.h>

#include "core/config.h"
#include "core/span.h"
#include "core/string.h"
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

using namespace soul::literals;

constexpr usize TEST_INLINE_CAPACITY = 32;
using TestString = soul::BasicString<soul::memory::Allocator, TEST_INLINE_CAPACITY>;

constexpr const char* TEST_SHORT_STR = "abcdef";
constexpr auto TEST_SHORT_STR_VIEW = soul::StringView{"abcdef"_str};
constexpr const auto TEST_SHORT_STR_SIZE = soul::str_length(TEST_SHORT_STR);
static_assert(TEST_SHORT_STR_SIZE + 1 < TEST_INLINE_CAPACITY);

constexpr const char* TEST_SHORT_STR2 = "adefghbc";
constexpr auto TEST_SHORT_STR_VIEW2 = soul::StringView{"abefghbc"_str};
constexpr const auto TEST_SHORT_STR_SIZE2 = soul::str_length(TEST_SHORT_STR2);
static_assert(TEST_SHORT_STR_SIZE2 + 1 < TEST_INLINE_CAPACITY);

constexpr const char* TEST_MAX_INLINE_STR = "abcdefghijklmnopqrstvuwxyz12345";
constexpr auto TEST_MAX_INLINE_STR_VIEW = soul::StringView{"abcdefghijklmnopqrstvuwxyz12345"_str};
static_assert(soul::str_length(TEST_MAX_INLINE_STR) == (TEST_INLINE_CAPACITY - 1));

constexpr const char* TEST_MAX_INLINE_STR2 = "12345abcdefghijklmnopqrstvuwxyz";
constexpr auto TEST_MAX_INLINE_STR_VIEW2 = soul::StringView{"12345abcdefghijklmnopqrstvuwxyz"_str};
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
constexpr const auto TEST_LONG_STR_VIEW = soul::StringView{TEST_LONG_STR, TEST_LONG_STR_SIZE};
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
constexpr const auto TEST_LONG_STR_VIEW2 = soul::StringView{TEST_LONG_STR2, TEST_LONG_STR2_SIZE};
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

template <typename StringT>
void verify_equal(const StringT& result_str, const char* expected_str)
{
  SOUL_TEST_ASSERT_STREQ(result_str.data(), expected_str);
  SOUL_TEST_ASSERT_EQ(result_str.size(), strlen(expected_str));
}

template <typename StringT>
void verify_equal(const StringT& result_str, soul::StringView expected_str_view)
{
  SOUL_TEST_ASSERT_STREQ(result_str.data(), expected_str_view.data());
  SOUL_TEST_ASSERT_EQ(result_str.size(), expected_str_view.size());
}

template <typename StringT>
void verify_equal(const StringT& result_str, const StringT& expected_str)
{
  SOUL_TEST_ASSERT_EQ(result_str, expected_str);
  SOUL_TEST_ASSERT_EQ(result_str.size(), expected_str.size());

  verify_equal(result_str, expected_str.data());
  SOUL_TEST_ASSERT_STREQ(result_str.data(), expected_str.data());
}

#include "common_test.h"

TEST(TestStringConstruction, TestDefaultConstructor)
{
  const TestString cstring;
  verify_equal(cstring, "");
}

TEST(TestStringConstruction, TestConstructionFromStringView)
{
  const auto test_construction_from = [](StringView str_view) {
    const auto cstring = TestString::From(str_view);
    SOUL_TEST_RUN(verify_equal(cstring, str_view));
  };

  SOUL_TEST_RUN(test_construction_from(""_str));
  SOUL_TEST_RUN(test_construction_from(TEST_SHORT_STR_VIEW));
  SOUL_TEST_RUN(test_construction_from(TEST_MAX_INLINE_STR_VIEW));
  SOUL_TEST_RUN(test_construction_from(TEST_LONG_STR_VIEW));
}

TEST(TestStringConstruction, TestStringConstructionFromLiteral)
{
  const auto literal_str = "abcdef"_str;
  SOUL_TEST_RUN(verify_equal(String(literal_str), String::From("abcdef")));
}

TEST(TestStringConstruction, TestConstructionWithSize)
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

TEST(TestStringConstruction, TestConstructionFormat)
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

TEST(TestStringConstruction, TestConstructionReservedFormat)
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

TEST(TestStringConstruction, TestConstructionWithCapacity)
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

TEST(TestStringConstruction, TestCustomAllocatorDefaultConstructor)
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

TEST(TestStringConstruction, TestClone)
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

TEST(TestStringConstruction, TestMoveConstructor)
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

class TestStringManipulation : public testing::Test
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

TEST_F(TestStringManipulation, TestMoveAssignemnt)
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

TEST_F(TestStringManipulation, TestAssignCompStr)
{
  const auto test_assign_comp_str = [](const TestString& sample_string, CompStr comp_str) {
    auto test_string = sample_string.clone();
    test_string = comp_str;
    SOUL_TEST_RUN(verify_equal(test_string, TestString(comp_str)));
  };
  SOUL_TEST_RUN(test_assign_comp_str(test_const_segment_string, "test"_str));
  SOUL_TEST_RUN(test_assign_comp_str(test_const_segment_string, ""_str));

  SOUL_TEST_RUN(test_assign_comp_str(test_short_string, "test"_str));
  SOUL_TEST_RUN(test_assign_comp_str(test_short_string, ""_str));

  SOUL_TEST_RUN(test_assign_comp_str(test_max_inline_string, "test"_str));
  SOUL_TEST_RUN(test_assign_comp_str(test_max_inline_string, ""_str));

  SOUL_TEST_RUN(test_assign_comp_str(test_long_string, "test"_str));
  SOUL_TEST_RUN(test_assign_comp_str(test_long_string, ""_str));

  SOUL_TEST_RUN(test_assign_comp_str(TestString(), "test"_str));
  SOUL_TEST_RUN(test_assign_comp_str(TestString(), ""_str));
}

TEST_F(TestStringManipulation, TestCloneFrom)
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

TEST_F(TestStringManipulation, TestStringSwap)
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

TEST_F(TestStringManipulation, TestReserve)
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

TEST_F(TestStringManipulation, TestClear)
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

TEST_F(TestStringManipulation, TestPushBack)
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

TEST_F(TestStringManipulation, TestAppendCharArr)
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

TEST_F(TestStringManipulation, TestAppend)
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

TEST_F(TestStringManipulation, TestAppendFormat)
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

TEST_F(TestStringManipulation, TestAssign)
{
  const auto test_assign = [](const TestString& sample_string, StringView assigned_str_view) {
    auto test_string = sample_string.clone();
    test_string.assign(assigned_str_view);
    verify_equal(test_string, assigned_str_view);
  };

  SOUL_TEST_RUN(test_assign(test_const_segment_string, TEST_SHORT_STR_VIEW));
  SOUL_TEST_RUN(test_assign(test_const_segment_string, TEST_MAX_INLINE_STR_VIEW));
  SOUL_TEST_RUN(test_assign(test_const_segment_string, TEST_LONG_STR_VIEW));
  SOUL_TEST_RUN(test_assign(test_const_segment_string, ""_str));

  SOUL_TEST_RUN(test_assign(test_short_string, TEST_SHORT_STR_VIEW));
  SOUL_TEST_RUN(test_assign(test_short_string, TEST_MAX_INLINE_STR_VIEW));
  SOUL_TEST_RUN(test_assign(test_short_string, TEST_LONG_STR_VIEW));
  SOUL_TEST_RUN(test_assign(test_short_string, ""_str));

  SOUL_TEST_RUN(test_assign(test_max_inline_string, TEST_SHORT_STR_VIEW));
  SOUL_TEST_RUN(test_assign(test_max_inline_string, TEST_MAX_INLINE_STR_VIEW));
  SOUL_TEST_RUN(test_assign(test_max_inline_string, TEST_LONG_STR_VIEW));
  SOUL_TEST_RUN(test_assign(test_max_inline_string, ""_str));

  SOUL_TEST_RUN(test_assign(test_long_string, TEST_SHORT_STR_VIEW));
  SOUL_TEST_RUN(test_assign(test_long_string, TEST_MAX_INLINE_STR_VIEW));
  SOUL_TEST_RUN(test_assign(test_long_string, TEST_LONG_STR_VIEW));
  SOUL_TEST_RUN(test_assign(test_long_string, ""_str));

  SOUL_TEST_RUN(test_assign(TestString(), TEST_SHORT_STR_VIEW));
  SOUL_TEST_RUN(test_assign(TestString(), TEST_MAX_INLINE_STR_VIEW));
  SOUL_TEST_RUN(test_assign(TestString(), TEST_LONG_STR_VIEW));
  SOUL_TEST_RUN(test_assign(TestString(), ""_str));
}

TEST_F(TestStringManipulation, TestAssignFormat)
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

TEST(TestStringFormat, TestStringFormat)
{
  SOUL_TEST_RUN(verify_equal(TestString::Format("{}", TestString()), ""));
  SOUL_TEST_RUN(verify_equal(TestString::Format("{}", TestString::From(TEST_SHORT_STR)), "abcdef"));
  SOUL_TEST_RUN(verify_equal(
    TestString::Format("{}", TestString::From(TEST_MAX_INLINE_STR)), TEST_MAX_INLINE_STR));
  SOUL_TEST_RUN(
    verify_equal(TestString::Format("{}", TestString::From(TEST_LONG_STR)), TEST_LONG_STR));
};

TEST(TestStringHash, TestStringHash)
{
  TestString test_const_segment_string = TestString::From(TEST_SHORT_STR);
  TestString test_const_segment_string2 = TestString::From(TEST_LONG_STR);

  TestString test_short_string = TestString::UnsharedFrom(TEST_SHORT_STR);
  TestString test_short_string2 = TestString::UnsharedFrom(TEST_SHORT_STR2);

  TestString test_max_inline_string = TestString::UnsharedFrom(TEST_MAX_INLINE_STR);
  TestString test_max_inline_string2 = TestString::UnsharedFrom(TEST_MAX_INLINE_STR2);

  TestString test_long_string = TestString::UnsharedFrom(TEST_LONG_STR);
  TestString test_long_string2 = TestString::UnsharedFrom(TEST_LONG_STR2);

  SOUL_TEST_ASSERT_EQ(soul::hash(test_const_segment_string), soul::hash(test_const_segment_string));
  SOUL_TEST_ASSERT_NE(
    soul::hash(test_const_segment_string), soul::hash(test_const_segment_string2));
  SOUL_TEST_ASSERT_EQ(soul::hash(test_const_segment_string), soul::hash(test_short_string));

  SOUL_TEST_ASSERT_EQ(soul::hash(test_short_string), soul::hash(test_short_string));
  SOUL_TEST_ASSERT_NE(soul::hash(test_short_string), soul::hash(test_short_string2));
  SOUL_TEST_ASSERT_NE(soul::hash(test_short_string), soul::hash(test_max_inline_string));
  SOUL_TEST_ASSERT_NE(soul::hash(test_short_string), soul::hash(test_long_string));

  SOUL_TEST_ASSERT_EQ(soul::hash(test_max_inline_string), soul::hash(test_max_inline_string));
  SOUL_TEST_ASSERT_NE(soul::hash(test_max_inline_string), soul::hash(test_max_inline_string2));
  SOUL_TEST_ASSERT_NE(soul::hash(test_max_inline_string), soul::hash(test_long_string));

  SOUL_TEST_ASSERT_EQ(soul::hash(test_long_string), soul::hash(test_long_string));
  SOUL_TEST_ASSERT_NE(soul::hash(test_long_string), soul::hash(test_long_string2));
}
