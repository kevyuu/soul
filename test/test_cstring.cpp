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

auto verify_cstring(const soul::CString& result_str, const char* expected_str) -> void
{
  SOUL_TEST_ASSERT_STREQ(result_str.data(), expected_str);
  SOUL_TEST_ASSERT_EQ(result_str.size(), strlen(expected_str));
}

auto verify_cstring(const soul::CString& result_str, const soul::CString& expected_str) -> void
{
  verify_cstring(result_str, expected_str.data());
  SOUL_TEST_ASSERT_STREQ(result_str.data(), expected_str.data());
  SOUL_TEST_ASSERT_EQ(result_str.size(), expected_str.size());
}

TEST(TestCStringConstructor, TestDefaultConstructor)
{
  const soul::CString cstring;
  SOUL_TEST_ASSERT_STREQ(cstring.data(), "");
  SOUL_TEST_ASSERT_EQ(cstring.size(), 0);
}

TEST(TestCStringConstructor, TestCharConstructor)
{
  const auto test_char_constructor = [](const char* str_literal) {
    const soul::CString cstring(str_literal);
    SOUL_TEST_RUN(verify_cstring(cstring, str_literal));
  };
  SOUL_TEST_RUN(test_char_constructor("test_char_constructor"));
  SOUL_TEST_RUN(test_char_constructor(""));
}

TEST(TestCStringConstructor, TestCustomAllocatorDefaultConstructor)
{
  TestAllocator::reset_all();
  TestAllocator test_allocator;
  soul::CString cstring(&test_allocator);
  SOUL_TEST_ASSERT_STREQ(cstring.data(), "");
  SOUL_TEST_ASSERT_EQ(cstring.size(), 0);

  const auto post_reserve_alloc_count = test_allocator.allocCount;
  cstring.reserve(10);
  SOUL_TEST_ASSERT_EQ(cstring.capacity(), 10);
  SOUL_TEST_ASSERT_EQ(test_allocator.allocCount - post_reserve_alloc_count, 1);
}

TEST(TestCStringConstructor, TestCopyConstructor)
{
  const auto test_copy_constructor = [](soul::CString src_string) {
    const soul::CString dst_string(src_string); // NOLINT
    SOUL_TEST_RUN(verify_cstring(dst_string, src_string.data()));
    SOUL_TEST_RUN(verify_cstring(dst_string, src_string));
  };

  SOUL_TEST_RUN(test_copy_constructor("test_copy_constructor"));
  SOUL_TEST_RUN(test_copy_constructor(""));
  SOUL_TEST_RUN(test_copy_constructor(soul::CString()));
}

TEST(TestCStringConstructor, TestMoveConstructor)
{
  const auto test_move_constructor = [](const char* str_literal) {
    soul::CString src_string(str_literal);
    const soul::CString dst_string(std::move(src_string));
    SOUL_TEST_RUN(verify_cstring(dst_string, str_literal));
  };

  SOUL_TEST_RUN(test_move_constructor("test_move_constructor"));
  SOUL_TEST_RUN(test_move_constructor(""));
}

TEST(TestCStringAssingmentOperator, TestCopyAssignmentOperator)
{
  const auto test_assignment_operator = [](const char* src_literal, const char* dst_literal) {
    const soul::CString src_string = src_literal;
    soul::CString dst_string = dst_literal;
    dst_string = src_string;
    SOUL_TEST_RUN(verify_cstring(dst_string, src_literal));
    SOUL_TEST_RUN(verify_cstring(dst_string, src_string));
  };
  SOUL_TEST_RUN(test_assignment_operator("test_src_cstring", "test_dst_cstring"));
  SOUL_TEST_RUN(test_assignment_operator("test_src_string", ""));
  SOUL_TEST_RUN(test_assignment_operator("", "test_dst_string"));
  SOUL_TEST_RUN(test_assignment_operator("", ""));
}

TEST(TestCStringAssignmentOperator, TestMoveAssignmentOperator)
{
  const auto test_move = [](soul::CString src_string, soul::CString dst_string) {
    const soul::CString copy_src_string = src_string;
    dst_string = std::move(src_string);
    verify_cstring(dst_string, copy_src_string.data());
  };
  SOUL_TEST_RUN(test_move(soul::CString("test_src_string"), soul::CString("test_dst_string")));
  SOUL_TEST_RUN(test_move(soul::CString(), soul::CString("test_dst_string")));
  SOUL_TEST_RUN(test_move(soul::CString("test_src_string"), soul::CString()));
  SOUL_TEST_RUN(test_move(soul::CString(), soul::CString()));
}

TEST(TestCStringSwap, TestCStringSwap)
{
  const auto test_swap = [](soul::CString str1, soul::CString str2) {
    auto str1copy = str1;
    auto str2copy = str2;

    str1copy.swap(str2copy);
    verify_cstring(str1copy, str2.data());
    verify_cstring(str1copy, str2);
    verify_cstring(str2copy, str1.data());
    verify_cstring(str2copy, str1);

    swap(str1copy, str2copy);
    verify_cstring(str1copy, str1.data());
    verify_cstring(str1copy, str1);
    verify_cstring(str2copy, str2.data());
    verify_cstring(str2copy, str2);
  };

  SOUL_TEST_RUN(test_swap("random", "testtesttest"));
  SOUL_TEST_RUN(test_swap("", "testtesttest"));
  SOUL_TEST_RUN(test_swap("testtest", ""));
  SOUL_TEST_RUN(test_swap(soul::CString(), ""));
  SOUL_TEST_RUN(test_swap("test", soul::CString()));
}

TEST(TestCStringReserve, TestCStringReserve)
{
  const auto test_reserve = [](soul::CString str, soul_size new_capacity) {
    const auto str_copy = str;
    str.reserve(new_capacity);
    SOUL_TEST_ASSERT_GE(str.capacity(), new_capacity);
  };

  SOUL_TEST_RUN(test_reserve("test", 0));
  SOUL_TEST_RUN(test_reserve("test", 4));
  SOUL_TEST_RUN(test_reserve("test", 8));
  SOUL_TEST_RUN(test_reserve("test", 100));

  SOUL_TEST_RUN(test_reserve("", 0));
  SOUL_TEST_RUN(test_reserve("", 4));
  SOUL_TEST_RUN(test_reserve("", 8));
  SOUL_TEST_RUN(test_reserve("", 100));

  SOUL_TEST_RUN(test_reserve(soul::CString(), 0));
  SOUL_TEST_RUN(test_reserve(soul::CString(), 4));
  SOUL_TEST_RUN(test_reserve(soul::CString(), 8));
  SOUL_TEST_RUN(test_reserve(soul::CString(), 100));
}

TEST(TestCStringPushBack, TestCStringPushBack)
{
  const auto test_push_back = [](soul::CString str, char c, const char* expected_str) {
    str.push_back(c);
    verify_cstring(str, expected_str);
  };

  SOUL_TEST_RUN(test_push_back("test ", 'x', "test x"));
  SOUL_TEST_RUN(test_push_back("a", 'z', "az"));
  SOUL_TEST_RUN(test_push_back("", 'y', "y"));
  SOUL_TEST_RUN(test_push_back(soul::CString(), 'k', "k"));
}

TEST(TestCStringAppend, TestAppendCharArr)
{
  const auto test_append = [](soul::CString str, const char* extra_str, const char* expected_str) {
    str.append(extra_str);
    verify_cstring(str, expected_str);
  };

  SOUL_TEST_RUN(test_append("test ", "append", "test append"));
  SOUL_TEST_RUN(test_append("test ", "", "test "));
  SOUL_TEST_RUN(test_append("", "append", "append"));
  SOUL_TEST_RUN(test_append("", "", ""));
  SOUL_TEST_RUN(test_append(soul::CString(), "append", "append"));
  SOUL_TEST_RUN(test_append(soul::CString(), "", ""));
}

TEST(TestCStringAppend, TestAppendCString)
{
  const auto test_append =
    [](soul::CString str, const soul::CString& extra_str, const soul::CString& expected_str) {
      str.append(extra_str);
      verify_cstring(str, expected_str);
    };

  SOUL_TEST_RUN(test_append("test ", "append", "test append"));
  SOUL_TEST_RUN(test_append("test ", "", "test "));
  SOUL_TEST_RUN(test_append("", "append", "append"));
  SOUL_TEST_RUN(test_append("", "", ""));
  SOUL_TEST_RUN(test_append(soul::CString(), "append", "append"));
  SOUL_TEST_RUN(test_append(soul::CString(), "", ""));
  SOUL_TEST_RUN(test_append(soul::CString(), soul::CString(), soul::CString()));
}
