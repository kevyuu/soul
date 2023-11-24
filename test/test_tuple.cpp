#include <gtest/gtest.h>

#include "core/config.h"
#include "core/meta.h"
#include "core/objops.h"
#include "core/panic.h"
#include "core/tuple.h"
#include "core/type_traits.h"
#include "core/views.h"
#include "memory/allocator.h"

#include "common_test.h"
#include "util.h"

namespace soul
{
  auto get_default_allocator() -> memory::Allocator*
  {
    static TestAllocator test_allocator("Test default allocator");
    return &test_allocator;
  }
} // namespace soul

struct TrivialObj {
  u8 x;
  u8 y;

  friend constexpr auto operator<=>(const TrivialObj& lhs, const TrivialObj& rhs) = default;
};

struct MoveOnlyObj {
  u8 x;
  u8 y;

  MoveOnlyObj(const MoveOnlyObj& other) = delete;
  MoveOnlyObj(MoveOnlyObj&& other) noexcept : x(other.x), y(other.y) {}
  auto operator=(const MoveOnlyObj& other) = delete;
  auto operator=(MoveOnlyObj&& other) noexcept -> MoveOnlyObj&
  {
    x = other.x;
    y = other.y;
    return *this;
  }

  ~MoveOnlyObj()
  {
    x = 0;
    y = 0;
  }
};

using TrivialTuple = soul::Tuple<u8, u16, TrivialObj>;
static_assert(soul::ts_copy<TrivialTuple>);
static_assert(
  soul::can_default_construct_v<u8> && soul::can_default_construct_v<u16> &&
  soul::can_default_construct_v<TrivialObj>);

using ListTestObject = soul::Vector<TestObject>;
using NontrivialTuple = soul::Tuple<ListTestObject, TestObject, u8>;
static_assert(soul::ts_clone<NontrivialTuple>);

using MoveOnlyTuple = soul::Tuple<u8, MoveOnlyObj>;
static_assert(soul::ts_move_only<MoveOnlyObj>);

struct EmptyObj {
};
static_assert(sizeof(soul::Tuple<u8, EmptyObj>) == sizeof(u8));

template <typename TupleT>
auto verify_equal(const TupleT& lhs, const TupleT& rhs)
{
  SOUL_TEST_ASSERT_EQ(lhs, rhs);
}

TEST(TestTupleConstruction, TestConstructionDefault)
{
  TrivialTuple trivial_tuple;

  NontrivialTuple nontrivial_tuple;
  SOUL_TEST_ASSERT_EQ(nontrivial_tuple.ref<0>(), ListTestObject());
  SOUL_TEST_ASSERT_EQ(nontrivial_tuple.ref<1>(), TestObject());
}

TEST(TestTupleConstruction, TestConstructionFromMember)
{
  {
    const auto trivial_obj = TrivialObj{3, 4};
    const auto trivial_tuple = TrivialTuple{1, 2, trivial_obj};
    SOUL_TEST_ASSERT_EQ(trivial_tuple.ref<0>(), 1);
    SOUL_TEST_ASSERT_EQ(trivial_tuple.ref<1>(), 2);
    SOUL_TEST_ASSERT_EQ(trivial_tuple.ref<2>(), trivial_obj);
  }

  {
    const auto test_list = ListTestObject::From(
      std::views::iota(0, 10) | std::views::transform([](int i) { return TestObject(i); }));

    NontrivialTuple nontrivial_tuple = {test_list.clone(), TestObject(3), 5};
    SOUL_TEST_ASSERT_EQ(nontrivial_tuple.ref<0>(), test_list);
    SOUL_TEST_ASSERT_EQ(nontrivial_tuple.ref<1>(), TestObject(3));
    SOUL_TEST_ASSERT_EQ(nontrivial_tuple.ref<2>(), 5);
  }

  {
    const auto test_tuple = soul::Tuple{8, EmptyObj{}};
  }
}

TEST(TestTupleConstruction, TestCopyConstructor)
{
  SOUL_TEST_RUN(test_copy_constructor(TrivialTuple{1, 2, TrivialObj{3, 4}}));
}

TEST(TestTupleConstruction, TestClone)
{
  const auto test_list = ListTestObject::From(
    std::views::iota(0, 10) | std::views::transform([](int i) { return TestObject(i); }));

  NontrivialTuple nontrivial_tuple = {test_list.clone(), TestObject(3), 5};
  SOUL_TEST_RUN(test_clone(nontrivial_tuple));
}

TEST(TestTupleConstruction, TestMoveConstructor)
{

  SOUL_TEST_RUN(test_move_constructor(TrivialTuple{1, 2, TrivialObj{3, 4}}));

  const auto test_list = ListTestObject::From(
    std::views::iota(0, 10) | std::views::transform([](int i) { return TestObject(i); }));
  NontrivialTuple nontrivial_tuple = {test_list.clone(), TestObject(3), 5};
  SOUL_TEST_RUN(test_move_constructor(nontrivial_tuple));
}

class TestTupleManipulation : public testing::Test
{
public:
  TrivialTuple trivial_tuple = TrivialTuple{1, 2, TrivialObj{3, 4}};
  TrivialTuple trivial_tuple2 = TrivialTuple{5, 6, TrivialObj{7, 8}};

  TestObject test_obj = TestObject(10);
  TestObject test_obj2 = TestObject(7);

  ListTestObject list_test_obj = ListTestObject::From(
    std::views::iota(3, 10) | std::views::transform([](i32 val) { return TestObject(val); }));
  ListTestObject list_test_obj2 = ListTestObject::From(
    std::views::iota(3, 7) | std::views::transform([](i32 val) { return TestObject(val); }));

  NontrivialTuple nontrivial_tuple = {list_test_obj.clone(), test_obj.clone(), 1};
  NontrivialTuple nontrivial_tuple2 = {list_test_obj2.clone(), test_obj2.clone(), 2};
};

TEST_F(TestTupleManipulation, TestCopyAssignment)
{
  SOUL_TEST_RUN(test_copy_assignment(trivial_tuple, trivial_tuple2));
}

TEST_F(TestTupleManipulation, TestMoveAssignment)
{
  SOUL_TEST_RUN(test_move_assignment(trivial_tuple, trivial_tuple2));
  SOUL_TEST_RUN(test_move_assignment(nontrivial_tuple, nontrivial_tuple2));
}

TEST_F(TestTupleManipulation, TestCloneFrom)
{
  SOUL_TEST_RUN(test_clone_from(nontrivial_tuple, nontrivial_tuple2));
}

TEST_F(TestTupleManipulation, TestSwap)
{
  SOUL_TEST_RUN(test_swap(trivial_tuple, trivial_tuple2));
  SOUL_TEST_RUN(test_swap(nontrivial_tuple, nontrivial_tuple2));
}
