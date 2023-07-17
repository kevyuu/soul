#include <gtest/gtest.h>

#include "common_test.h"
#include "core/config.h"
#include "core/not_null.h"
#include "core/objops.h"
#include "core/option.h"
#include "core/type_traits.h"
#include "core/views.h"
#include "memory/allocator.h"

#include "util.h"

using namespace soul;

namespace soul
{
  template <ts_maybe_null T>
  void verify_equal(const T& lhs, const T& rhs)
  {
    SOUL_TEST_ASSERT_EQ(lhs.some_ref(), rhs.some_ref());
    typename T::ptr_type lhs_ptr = lhs;
    typename T::ptr_type rhs_ptr = rhs;
    SOUL_TEST_ASSERT_EQ(lhs_ptr, rhs_ptr);
    SOUL_TEST_ASSERT_EQ(lhs, rhs);
  }
} // namespace soul

TEST(TestMaybeNullConstruction, TestConstruction)
{
  i32 test_int = 3;

  {
    MaybeNull<i32*> test_maybe_null = &test_int;
    SOUL_TEST_ASSERT_EQ(test_maybe_null.unwrap(), ptrof(test_int));
    SOUL_TEST_ASSERT_EQ(test_maybe_null.some_ref(), ptrof(test_int));
    SOUL_TEST_ASSERT_TRUE(test_maybe_null.is_some());
    SOUL_TEST_ASSERT_EQ(test_maybe_null, &test_int);
  }

  {
    i32* test_int_ptr = nullptr;
    MaybeNull<i32*> test_maybe_null = nullptr;
    SOUL_TEST_ASSERT_FALSE(test_maybe_null.is_some());
    SOUL_TEST_ASSERT_EQ(test_maybe_null, nullptr);
  }

  {
    const auto test_not_null = ptrof(test_int);
    const auto test_maybe_null = MaybeNull<i32*>::some(test_not_null);
    SOUL_TEST_ASSERT_EQ(test_maybe_null.unwrap(), ptrof(test_int));
    SOUL_TEST_ASSERT_EQ(test_maybe_null.some_ref(), ptrof(test_int));
    SOUL_TEST_ASSERT_TRUE(test_maybe_null.is_some());
  }

  {
    const MaybeNull<i32*> test_maybe_null = none;
    SOUL_TEST_ASSERT_EQ(test_maybe_null, nullptr);
  }

  test_copy_constructor(MaybeNull<i32*>::some(ptrof(test_int)));
  test_copy_constructor(MaybeNull<i32*>(none));

  test_move_constructor(MaybeNull<i32*>::some(ptrof(test_int)));
  test_move_constructor(MaybeNull<i32*>(none));
}

TEST(TestMaybeNullManipulation, TestManipulation)
{
  i32 test_int = 3;
  i32 test_int2 = 5;
  const auto test_maybe_null_some = MaybeNull<i32*>::some(ptrof(test_int));
  const auto test_maybe_null_some2 = MaybeNull<i32*>::some(ptrof(test_int2));
  const auto test_maybe_null_none = MaybeNull<i32*>();

  test_copy_assignment(test_maybe_null_some, test_maybe_null_some2);
  test_copy_assignment(test_maybe_null_none, test_maybe_null_some);
  test_copy_assignment(test_maybe_null_some, test_maybe_null_none);
  test_copy_assignment(test_maybe_null_none, test_maybe_null_none);

  test_move_assignment(test_maybe_null_some, test_maybe_null_some2);
  test_move_assignment(test_maybe_null_none, test_maybe_null_some);
  test_move_assignment(test_maybe_null_some, test_maybe_null_none);
  test_move_assignment(test_maybe_null_none, test_maybe_null_none);

  test_swap(test_maybe_null_some, test_maybe_null_some2);
  test_swap(test_maybe_null_none, test_maybe_null_some);
  test_swap(test_maybe_null_some, test_maybe_null_none);
  test_swap(test_maybe_null_none, test_maybe_null_none);
}

TEST(TestMaybeNullMonadic, TestMonadic)
{
  i32 test_int = 3;
  i32 test_int2 = 5;
  const auto test_maybe_null_some = MaybeNull<i32*>::some(ptrof(test_int));
  const auto test_maybe_null_some2 = MaybeNull<i32*>::some(ptrof(test_int2));
  const auto test_maybe_null_none = MaybeNull<i32*>();

  {
    auto is_some_fn = [&test_int](const NotNull<i32*>& val) { return *val == test_int; };
    SOUL_TEST_ASSERT_TRUE(test_maybe_null_some.is_some_and(is_some_fn));
    SOUL_TEST_ASSERT_FALSE(test_maybe_null_some2.is_some_and(is_some_fn));
    SOUL_TEST_ASSERT_FALSE(test_maybe_null_none.is_some_and(is_some_fn));
  }

  {
    SOUL_TEST_ASSERT_EQ(test_maybe_null_some.unwrap_or(&test_int2), ptrof(test_int));
    SOUL_TEST_ASSERT_EQ(test_maybe_null_none.unwrap_or(&test_int2), ptrof(test_int2));
  }

  {
    auto unwrap_or_else_fn = [&test_int2]() { return &test_int2; };
    SOUL_TEST_ASSERT_EQ(test_maybe_null_some.unwrap_or_else(unwrap_or_else_fn), ptrof(test_int));
    SOUL_TEST_ASSERT_EQ(test_maybe_null_none.unwrap_or_else(unwrap_or_else_fn), ptrof(test_int2));
  }

  {
    auto and_then_fn = [&test_int2](NotNull<i32*> /* val */) {
      return MaybeNull<i32*>::some(&test_int2);
    };
    SOUL_TEST_ASSERT_EQ(test_maybe_null_some.and_then(and_then_fn), ptrof(test_int2));
    SOUL_TEST_ASSERT_FALSE(test_maybe_null_none.and_then(and_then_fn).is_some());
  }

  {
    auto transform_fn = [&test_int2](NotNull<i32*> /* val */) { return ptrof(test_int2); };
    SOUL_TEST_ASSERT_EQ(test_maybe_null_some.transform(transform_fn), ptrof(test_int2));
    SOUL_TEST_ASSERT_FALSE(test_maybe_null_none.transform(transform_fn).is_some());
  }

  {
    auto or_else_fn = [&test_int2]() { return MaybeNull<i32*>::some(&test_int2); };
    SOUL_TEST_ASSERT_EQ(test_maybe_null_some.or_else(or_else_fn), ptrof(test_int));
    SOUL_TEST_ASSERT_EQ(test_maybe_null_none.or_else(or_else_fn), ptrof(test_int2));
  }
}

TEST(TestMaybeNullReset, TestReset)
{
  i32 test_int = 3;
  auto test_maybe_null_some = MaybeNull<i32*>::some(ptrof(test_int));
  test_maybe_null_some.reset();
  SOUL_TEST_ASSERT_FALSE(test_maybe_null_some.is_some());

  auto test_maybe_null_none = MaybeNull<i32*>();
  test_maybe_null_none.reset();
  SOUL_TEST_ASSERT_FALSE(test_maybe_null_none.is_some());
}

static_assert(!is_maybe_null_v<i32>);
static_assert(is_maybe_null_v<MaybeNull<i32*>>);
static_assert(is_maybe_null_v<MaybeNull<i32*>, i32*>);
static_assert(is_maybe_null_v<MaybeNull<i32*>, match_any>);
static_assert(!is_maybe_null_v<MaybeNull<i32*>, i64*>);

static_assert(!ts_maybe_null<i32>);
static_assert(ts_maybe_null<MaybeNull<i32*>>);
static_assert(ts_maybe_null<MaybeNull<i32*>, i32*>);
static_assert(ts_maybe_null<MaybeNull<i32*>, match_any>);
static_assert(!ts_maybe_null<MaybeNull<i32*>, i64*>);
