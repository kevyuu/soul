#include "core/comp_str.h"

#include "util.h"

using namespace soul;
TEST(TestStrConstruction, TestStrConstruction)
{
  const auto test_str1 = "test"_str;
  SOUL_TEST_ASSERT_EQ(test_str1.size(), 4);
  SOUL_TEST_ASSERT_EQ(test_str1.data()[0], 't');
  SOUL_TEST_ASSERT_EQ(test_str1.data()[1], 'e');
  SOUL_TEST_ASSERT_EQ(test_str1.data()[2], 's');
  SOUL_TEST_ASSERT_EQ(test_str1.data()[3], 't');
  SOUL_TEST_ASSERT_EQ(test_str1.data()[4], '\0');

  const auto test_str2 = ""_str;
  SOUL_TEST_ASSERT_EQ(test_str2.size(), 0);
  SOUL_TEST_ASSERT_EQ(test_str2.data()[0], '\0');
}
