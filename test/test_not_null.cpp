#include <gtest/gtest.h>

#include "core/config.h"
#include "core/not_null.h"
#include "core/objops.h"
#include "core/type_traits.h"
#include "core/views.h"
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

TEST(TestNotNullConstruction, TestConstructionFromRawPointer)
{
  const auto test_obj = TestObject(4);
  const soul::NotNull<const TestObject*> not_null_test_obj = &test_obj;
  SOUL_TEST_ASSERT_EQ(*not_null_test_obj, test_obj);
  SOUL_TEST_ASSERT_EQ(not_null_test_obj, &test_obj);
  SOUL_TEST_ASSERT_EQ(not_null_test_obj->x, test_obj.x);
  SOUL_TEST_ASSERT_EQ((*not_null_test_obj).x, test_obj.x);
}

TEST(TestNotNullConstruction, TestPtrOf)
{
  const auto test_obj = TestObject(4);
  const auto not_null_test_obj = soul::ptrof(test_obj);
  SOUL_TEST_ASSERT_EQ(*not_null_test_obj, test_obj);
  SOUL_TEST_ASSERT_EQ(not_null_test_obj, &test_obj);
  SOUL_TEST_ASSERT_EQ(not_null_test_obj->x, test_obj.x);
  SOUL_TEST_ASSERT_EQ((*not_null_test_obj).x, test_obj.x);
}

TEST(TestNotNullConstruction, TestCopyConstructor)
{
  const auto test_obj = TestObject(4);
  const auto not_null_test_obj = soul::ptrof(test_obj);
  const auto not_null_test_obj2 = not_null_test_obj;

  SOUL_TEST_ASSERT_EQ(*not_null_test_obj2, test_obj);
  SOUL_TEST_ASSERT_EQ(not_null_test_obj2, &test_obj);
  SOUL_TEST_ASSERT_EQ(not_null_test_obj2->x, test_obj.x);
  SOUL_TEST_ASSERT_EQ((*not_null_test_obj2).x, test_obj.x);

  SOUL_TEST_ASSERT_EQ(not_null_test_obj, not_null_test_obj2);
}

TEST(TestNotNullAssignment, TestAssignment)
{
  const auto test_obj = TestObject(4);
  auto not_null_test_obj = soul::ptrof(test_obj);

  const auto test_obj2 = TestObject(5);
  not_null_test_obj = soul::ptrof(test_obj2);

  SOUL_TEST_ASSERT_EQ(*not_null_test_obj, test_obj2);
  SOUL_TEST_ASSERT_EQ(not_null_test_obj, &test_obj2);
  SOUL_TEST_ASSERT_EQ(not_null_test_obj->x, test_obj2.x);
  SOUL_TEST_ASSERT_EQ((*not_null_test_obj).x, test_obj2.x);
}