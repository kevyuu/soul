#include <algorithm>
#include <numeric>
#include <random>
#include <xutility>

#include <gtest/gtest.h>

#include "core/array.h"
#include "core/config.h"
#include "core/deque.h"
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

using DequeInt = soul::Deque<int>;
static_assert(std::ranges::bidirectional_range<DequeInt>);
using DequeObj = soul::Deque<TestObject>;
static_assert(std::ranges::bidirectional_range<DequeObj>);
using DequeListObj = soul::Deque<ListTestObject>;
static_assert(std::ranges::bidirectional_range<DequeListObj>);

template <typename DequeT>
auto verify_equal(const DequeT& lhs, const DequeT& rhs)
{
  using value_t = DequeT::value_type;
  SOUL_TEST_ASSERT_EQ(lhs.size(), rhs.size());
  for (auto i = 0; i < lhs.size(); i++)
  {
    if (lhs[i] != rhs[i])
    {

      SOUL_LOG_INFO("i : {}", i);
    }
    SOUL_TEST_ASSERT_EQ(lhs[i], rhs[i]);
  }
  if (lhs.size() > 0)
  {
    SOUL_TEST_ASSERT_EQ(lhs.back_ref(), rhs.back_ref());
    SOUL_TEST_ASSERT_EQ(lhs.front_ref(), rhs.front_ref());
  }
}

#include "common_test.h"

template <typename T>
auto test_default_constructor() -> void
{
  soul::Deque<T> deque;
  SOUL_TEST_ASSERT_TRUE(deque.size() == 0);
  SOUL_TEST_ASSERT_TRUE(deque.empty());
}

TEST(TestDequeConstruction, TestDefaultConstructor)
{
  SOUL_TEST_RUN(test_default_constructor<int>());
  SOUL_TEST_RUN(test_default_constructor<TestObject>());
  SOUL_TEST_RUN(test_default_constructor<ListTestObject>());
}

template <typename DequeT>
auto test_construction_with_capacity(usize capacity)
{
  const auto test_deque = DequeT::WithCapacity(capacity);
  SOUL_TEST_ASSERT_EQ(test_deque.size(), 0);
  SOUL_TEST_ASSERT_TRUE(test_deque.empty());
  SOUL_TEST_ASSERT_GE(test_deque.capacity(), capacity);
}

TEST(TestDequeConstruction, TestConstructionWithCapacity)
{
  SOUL_TEST_RUN(test_construction_with_capacity<DequeInt>(0));
  SOUL_TEST_RUN(test_construction_with_capacity<DequeInt>(10));
  SOUL_TEST_RUN(test_construction_with_capacity<DequeObj>(0));
  SOUL_TEST_RUN(test_construction_with_capacity<DequeObj>(10));
  SOUL_TEST_RUN(test_construction_with_capacity<DequeListObj>(0));
  SOUL_TEST_RUN(test_construction_with_capacity<DequeListObj>(10));
}

template <std::ranges::forward_range RangeT>
auto test_construction_from_range(const RangeT& entries)
{
  using range_value_t = std::ranges::range_value_t<RangeT>;
  auto entry_vector =
    soul::Vector<range_value_t>::From(entries | soul::views::duplicate<range_value_t>());

  const auto test_deque =
    soul::Deque<range_value_t>::From(entries | soul::views::duplicate<range_value_t>());

  SOUL_TEST_ASSERT_EQ(entry_vector.size(), test_deque.size());
  SOUL_TEST_ASSERT_TRUE(std::ranges::equal(test_deque, entry_vector));
}

TEST(TestDequeConstruction, TestConstructionFromRange)
{
  SOUL_TEST_RUN(test_construction_from_range(soul::Array<int, 0>{}));
  SOUL_TEST_RUN(test_construction_from_range(soul::Array{3, 10, 1000}));
  SOUL_TEST_RUN(test_construction_from_range(
    soul::Array{3, 4, 5, 6, 7, 30, 31, 32, 33, 34, 35, 36, 37, 37, 37, 10, 1000}));
  SOUL_TEST_RUN(test_construction_from_range(generate_random_sequence<int>(1000)));

  SOUL_TEST_RUN(test_construction_from_range(soul::Array<TestObject, 0>{}));
  SOUL_TEST_RUN(
    test_construction_from_range(soul::Array{TestObject(3), TestObject(10), TestObject(1000)}));
  SOUL_TEST_RUN(test_construction_from_range(generate_random_sequence<TestObject>(500)));

  // SOUL_TEST_RUN(test_construction_from_range(generate_random_sequence<ListTestObject>(100)));
}

class TestDequeManipulationAfterConstruction : public testing::Test
{
public:
  DequeInt deque_int1 = DequeInt::From(generate_random_sequence<int>(1000));
  DequeInt deque_int2 = DequeInt::From(generate_random_sequence<int>(8));

  DequeObj deque_obj1 =
    DequeObj::From(generate_random_sequence<TestObject>(5) | soul::views::clone<TestObject>());
  DequeObj deque_obj2 =
    DequeObj::From(generate_random_sequence<TestObject>(100) | soul::views::clone<TestObject>());
};

TEST_F(TestDequeManipulationAfterConstruction, TestClone)
{
  SOUL_TEST_RUN(test_clone(DequeInt()));
  SOUL_TEST_RUN(test_clone(deque_int1));
  SOUL_TEST_RUN(test_clone(deque_obj1));
}

TEST_F(TestDequeManipulationAfterConstruction, TestCloneFrom)
{
  SOUL_TEST_RUN(test_clone_from(deque_int1, deque_int2));
  SOUL_TEST_RUN(test_clone_from(deque_int2, deque_int1));
  SOUL_TEST_RUN(test_clone_from(DequeInt(), deque_int1));
  SOUL_TEST_RUN(test_clone_from(deque_int1, DequeInt()));
  SOUL_TEST_RUN(test_clone_from(DequeInt(), deque_int2));
  SOUL_TEST_RUN(test_clone_from(deque_int2, DequeInt()));
  SOUL_TEST_RUN(test_clone_from(DequeInt(), deque_int2));
  SOUL_TEST_RUN(test_clone_from(DequeInt(), DequeInt()));

  SOUL_TEST_RUN(test_clone_from(deque_obj1, deque_obj2));
  SOUL_TEST_RUN(test_clone_from(deque_obj2, deque_obj1));
  SOUL_TEST_RUN(test_clone_from(DequeObj(), deque_obj1));
  SOUL_TEST_RUN(test_clone_from(deque_obj1, DequeObj()));
  SOUL_TEST_RUN(test_clone_from(DequeObj(), deque_obj2));
  SOUL_TEST_RUN(test_clone_from(deque_obj2, DequeObj()));
  SOUL_TEST_RUN(test_clone_from(DequeObj(), deque_obj2));
  SOUL_TEST_RUN(test_clone_from(DequeObj(), DequeObj()));
}

TEST_F(TestDequeManipulationAfterConstruction, TestMoveConstructor)
{
  SOUL_TEST_RUN(test_move_constructor(DequeInt()));
  SOUL_TEST_RUN(test_move_constructor(deque_int1));
  SOUL_TEST_RUN(test_move_constructor(deque_obj1));
}

TEST_F(TestDequeManipulationAfterConstruction, TestMoveAssignment)
{
  SOUL_TEST_RUN(test_move_assignment(deque_int1, deque_int2));
  SOUL_TEST_RUN(test_move_assignment(deque_int2, deque_int1));
  SOUL_TEST_RUN(test_move_assignment(DequeInt(), deque_int1));
  SOUL_TEST_RUN(test_move_assignment(deque_int1, DequeInt()));
  SOUL_TEST_RUN(test_move_assignment(DequeInt(), deque_int2));
  SOUL_TEST_RUN(test_move_assignment(deque_int2, DequeInt()));
  SOUL_TEST_RUN(test_move_assignment(DequeInt(), deque_int2));
  SOUL_TEST_RUN(test_move_assignment(DequeInt(), DequeInt()));

  SOUL_TEST_RUN(test_move_assignment(deque_obj1, deque_obj2));
  SOUL_TEST_RUN(test_move_assignment(deque_obj2, deque_obj1));
  SOUL_TEST_RUN(test_move_assignment(DequeObj(), deque_obj1));
  SOUL_TEST_RUN(test_move_assignment(deque_obj1, DequeObj()));
  SOUL_TEST_RUN(test_move_assignment(DequeObj(), deque_obj2));
  SOUL_TEST_RUN(test_move_assignment(deque_obj2, DequeObj()));
  SOUL_TEST_RUN(test_move_assignment(DequeObj(), deque_obj2));
  SOUL_TEST_RUN(test_move_assignment(DequeObj(), DequeObj()));
}

TEST_F(TestDequeManipulationAfterConstruction, TestSwap)
{
  SOUL_TEST_RUN(test_swap(deque_int1, deque_int2));
  SOUL_TEST_RUN(test_swap(deque_int2, deque_int1));
  SOUL_TEST_RUN(test_swap(DequeInt(), deque_int1));
  SOUL_TEST_RUN(test_swap(deque_int1, DequeInt()));
  SOUL_TEST_RUN(test_swap(DequeInt(), deque_int2));
  SOUL_TEST_RUN(test_swap(deque_int2, DequeInt()));
  SOUL_TEST_RUN(test_swap(DequeInt(), deque_int2));
  SOUL_TEST_RUN(test_swap(DequeInt(), DequeInt()));

  SOUL_TEST_RUN(test_swap(deque_obj1, deque_obj2));
  SOUL_TEST_RUN(test_swap(deque_obj2, deque_obj1));
  SOUL_TEST_RUN(test_swap(DequeObj(), deque_obj1));
  SOUL_TEST_RUN(test_swap(deque_obj1, DequeObj()));
  SOUL_TEST_RUN(test_swap(DequeObj(), deque_obj2));
  SOUL_TEST_RUN(test_swap(deque_obj2, DequeObj()));
  SOUL_TEST_RUN(test_swap(DequeObj(), deque_obj2));
  SOUL_TEST_RUN(test_swap(DequeObj(), DequeObj()));
}

template <typename DequeT>
void test_deque_clear(const DequeT& deque)
{
  auto test_deque = deque.clone();
  test_deque.clear();

  SOUL_TEST_ASSERT_EQ(test_deque.size(), 0);
  SOUL_TEST_ASSERT_TRUE(test_deque.empty());
  SOUL_TEST_ASSERT_EQ(test_deque.begin(), test_deque.end());
};

TEST_F(TestDequeManipulationAfterConstruction, TestClear)
{

  SOUL_TEST_RUN(test_deque_clear(DequeInt()));
  SOUL_TEST_RUN(test_deque_clear(deque_int1));
  SOUL_TEST_RUN(test_deque_clear(deque_obj1));
}

template <typename DequeT>
void test_deque_cleanup(const DequeT& deque)
{
  auto test_deque = deque.clone();
  test_deque.cleanup();

  SOUL_TEST_ASSERT_EQ(test_deque.size(), 0);
  SOUL_TEST_ASSERT_TRUE(test_deque.empty());
  SOUL_TEST_ASSERT_EQ(test_deque.begin(), test_deque.end());
  SOUL_TEST_ASSERT_EQ(test_deque.capacity(), 0);
};

TEST_F(TestDequeManipulationAfterConstruction, TestCleanup)
{
  SOUL_TEST_RUN(test_deque_cleanup(DequeInt()));
  SOUL_TEST_RUN(test_deque_cleanup(deque_int1));
  SOUL_TEST_RUN(test_deque_cleanup(deque_obj1));
}

TEST_F(TestDequeManipulationAfterConstruction, TestReserve)
{
  SOUL_TEST_RUN(test_reserve(DequeInt(), 10));
  SOUL_TEST_RUN(test_reserve(deque_int1, 0));
  SOUL_TEST_RUN(test_reserve(deque_int1, 10));
  SOUL_TEST_RUN(test_reserve(deque_int2, 0));
  SOUL_TEST_RUN(test_reserve(deque_int2, 1));
  SOUL_TEST_RUN(test_reserve(deque_int2, deque_int2.size() / 2));
  SOUL_TEST_RUN(test_reserve(deque_int2, deque_int2.size() * 2));

  SOUL_TEST_RUN(test_reserve(DequeObj(), 10));
  SOUL_TEST_RUN(test_reserve(deque_obj1, 0));
  SOUL_TEST_RUN(test_reserve(deque_obj1, 10));
  SOUL_TEST_RUN(test_reserve(deque_obj2, 0));
  SOUL_TEST_RUN(test_reserve(deque_obj2, 1));
  SOUL_TEST_RUN(test_reserve(deque_obj2, deque_obj2.size() / 2));
  SOUL_TEST_RUN(test_reserve(deque_obj2, deque_obj2.size() * 2));
}

template <typename T>
void test_deque_shrink_to_fit(const soul::Deque<T>& sample_deque, const usize new_capacity)
{
  auto test_deque         = sample_deque.clone();
  const auto old_capacity = test_deque.capacity();
  test_deque.reserve(new_capacity);
  test_deque.shrink_to_fit();
  SOUL_TEST_ASSERT_TRUE(std::ranges::equal(test_deque, sample_deque));
  SOUL_TEST_ASSERT_EQ(test_deque.capacity(), sample_deque.size());
};

TEST_F(TestDequeManipulationAfterConstruction, TestShrinkToFit)
{
  SOUL_TEST_RUN(test_deque_shrink_to_fit(DequeInt(), 5));
  SOUL_TEST_RUN(test_deque_shrink_to_fit(DequeObj(), 5));
  SOUL_TEST_RUN(test_deque_shrink_to_fit(DequeListObj(), 5));

  SOUL_TEST_RUN(test_deque_shrink_to_fit(deque_int1, deque_int1.capacity()));
  SOUL_TEST_RUN(test_deque_shrink_to_fit(deque_obj1, deque_obj1.capacity() + 5));
}

template <typename T>
void test_deque_push_back(const soul::Deque<T>& sample_deque, const T& val)
{
  using Deque = soul::Deque<T>;

  auto test_deque  = sample_deque.clone();
  Deque test_copy1 = test_deque.clone();
  Deque test_copy2 = test_deque.clone();

  if constexpr (!soul::ts_clone<T>)
  {
    test_copy1.push_back(val);
    SOUL_TEST_ASSERT_EQ(test_copy1.size(), test_deque.size() + 1);
    for (usize i = 0; i < test_deque.size(); i++)
    {
      if (test_deque[i] != test_copy1[i])
      {
        SOUL_LOG_INFO(
          "idx : {}, test_deque item : {}, test_copy1 item : {}", i, test_deque[i], test_copy1[i]);
      }
    }
    SOUL_TEST_ASSERT_TRUE(std::equal(test_deque.begin(), test_deque.end(), test_copy1.begin()));
    SOUL_TEST_ASSERT_EQ(test_copy1.back_ref(), val);
  }

  T val_copy = soul::duplicate(val);
  test_copy2.push_back(std::move(val_copy));
  SOUL_TEST_ASSERT_EQ(test_copy2.size(), test_deque.size() + 1);
  SOUL_TEST_ASSERT_TRUE(std::equal(test_deque.begin(), test_deque.end(), test_copy2.begin()));
  SOUL_TEST_ASSERT_EQ(test_copy2.back_ref(), val);
};

TEST_F(TestDequeManipulationAfterConstruction, TestPushBack)
{

  SOUL_TEST_RUN(test_deque_push_back(DequeInt(), 5));
  SOUL_TEST_RUN(test_deque_push_back(DequeObj(), TestObject(5)));

  SOUL_TEST_RUN(test_deque_push_back(deque_int1, 5));
  SOUL_TEST_RUN(test_deque_push_back(deque_obj1, TestObject(5)));

  auto test_push_back_self_referential = []<typename T>(const soul::Deque<T>& sample_deque)
  {
    using Deque = soul::Deque<T>;

    auto test_deque = sample_deque.clone();
    test_deque.reserve(test_deque.capacity() + 10);
    test_deque.shrink_to_fit();
    test_deque.push_back(soul::duplicate(test_deque.back_ref()));

    SOUL_TEST_ASSERT_EQ(test_deque.size(), sample_deque.size() + 1);
    SOUL_TEST_ASSERT_TRUE(std::equal(sample_deque.begin(), sample_deque.end(), test_deque.begin()));
    SOUL_TEST_ASSERT_EQ(test_deque.back_ref(), sample_deque.back_ref());
  };

  SOUL_TEST_RUN(test_push_back_self_referential(deque_obj1));
}

template <typename T>
void test_deque_push_front(const soul::Deque<T>& sample_deque, const T& val)
{
  using Deque = soul::Deque<T>;

  auto test_deque  = sample_deque.clone();
  Deque test_copy1 = test_deque.clone();
  Deque test_copy2 = test_deque.clone();

  if constexpr (!soul::ts_clone<T>)
  {
    test_copy1.push_front(val);
    SOUL_TEST_ASSERT_EQ(test_copy1.size(), test_deque.size() + 1);
    SOUL_TEST_ASSERT_TRUE(std::equal(test_deque.rbegin(), test_deque.rend(), test_copy1.rbegin()));
    SOUL_TEST_ASSERT_EQ(test_copy1.front_ref(), val);
  }

  T val_copy = soul::duplicate(val);
  test_copy2.push_front(std::move(val_copy));
  SOUL_TEST_ASSERT_EQ(test_copy2.size(), test_deque.size() + 1);
  SOUL_TEST_ASSERT_TRUE(std::equal(test_deque.rbegin(), test_deque.rend(), test_copy2.rbegin()));
  SOUL_TEST_ASSERT_EQ(test_copy2.front_ref(), val);
};

TEST_F(TestDequeManipulationAfterConstruction, TestPushFront)
{
  SOUL_TEST_RUN(test_deque_push_front(DequeInt(), 5));
  SOUL_TEST_RUN(test_deque_push_front(DequeObj(), TestObject(5)));

  SOUL_TEST_RUN(test_deque_push_front(deque_int1, 5));
  SOUL_TEST_RUN(test_deque_push_front(deque_obj1, TestObject(5)));

  auto test_push_front_self_referential = []<typename T>(const soul::Deque<T>& sample_deque)
  {
    using Deque = soul::Deque<T>;

    auto test_deque = sample_deque.clone();
    test_deque.reserve(test_deque.capacity() + 10);
    test_deque.shrink_to_fit();
    test_deque.push_front(soul::duplicate(test_deque.front_ref()));

    SOUL_TEST_ASSERT_EQ(test_deque.size(), sample_deque.size() + 1);
    SOUL_TEST_ASSERT_TRUE(
      std::equal(sample_deque.rbegin(), sample_deque.rend(), test_deque.rbegin()));
    SOUL_TEST_ASSERT_EQ(test_deque.front_ref(), sample_deque.front_ref());
  };

  SOUL_TEST_RUN(test_push_front_self_referential(deque_obj1));
}

template <typename T>
void test_deque_pop_front(const soul::Deque<T>& sample_deque)
{
  auto test_deque = sample_deque.clone();

  for (const auto& item : sample_deque)
  {
    SOUL_TEST_ASSERT_EQ(test_deque.pop_front(), item);
  }
  SOUL_TEST_ASSERT_EQ(test_deque.size(), 0);
  SOUL_TEST_ASSERT_TRUE(test_deque.empty());
  SOUL_TEST_ASSERT_EQ(test_deque.begin(), test_deque.end());
};

TEST_F(TestDequeManipulationAfterConstruction, TestDequePopFront)
{
  SOUL_TEST_RUN(test_deque_pop_front(DequeInt()));
  SOUL_TEST_RUN(test_deque_pop_front(DequeObj()));

  SOUL_TEST_RUN(test_deque_pop_front(deque_int1));
  SOUL_TEST_RUN(test_deque_pop_front(deque_obj1));
}

template <typename T>
void test_deque_pop_back(const soul::Deque<T>& sample_deque)
{
  auto test_deque = sample_deque.clone();

  for (const auto& item : sample_deque | std::views::reverse)
  {
    SOUL_TEST_ASSERT_EQ(test_deque.pop_back(), item);
  }
  SOUL_TEST_ASSERT_EQ(test_deque.size(), 0);
  SOUL_TEST_ASSERT_TRUE(test_deque.empty());
  SOUL_TEST_ASSERT_EQ(test_deque.begin(), test_deque.end());
};

TEST_F(TestDequeManipulationAfterConstruction, TestDequePopBack)
{
  SOUL_TEST_RUN(test_deque_pop_back(DequeInt()));

  SOUL_TEST_RUN(test_deque_pop_back(deque_int1));
  SOUL_TEST_RUN(test_deque_pop_back(deque_obj1));
}

class TestDequeManipulationAfterManipulation : public testing::Test
{
public:
  DequeInt deque_after_push_back  = DequeInt::From(generate_random_sequence<int>(1000));
  DequeInt deque_after_push_front = DequeInt::From(generate_random_sequence<int>(8));

  DequeObj deque_after_pop_front =
    DequeObj::From(generate_random_sequence<TestObject>(5) | soul::views::clone<TestObject>());
  DequeObj deque_after_pop_back =
    DequeObj::From(generate_random_sequence<TestObject>(100) | soul::views::clone<TestObject>());

  TestDequeManipulationAfterManipulation()
  {
    deque_after_push_back.push_back(5);
    deque_after_push_front.push_front(10);

    deque_after_pop_front.pop_front();
    deque_after_pop_back.pop_back();
  }
};

TEST_F(TestDequeManipulationAfterManipulation, TestClone)
{
  SOUL_TEST_RUN(test_clone(deque_after_push_back));
  SOUL_TEST_RUN(test_clone(deque_after_push_front));
  SOUL_TEST_RUN(test_clone(deque_after_pop_front));
  SOUL_TEST_RUN(test_clone(deque_after_pop_back));
}

TEST_F(TestDequeManipulationAfterManipulation, TestCloneFrom)
{
  SOUL_TEST_RUN(test_clone_from(deque_after_push_back, deque_after_push_front));
  SOUL_TEST_RUN(test_clone_from(deque_after_push_front, deque_after_push_back));
  SOUL_TEST_RUN(test_clone_from(DequeInt(), deque_after_push_back));
  SOUL_TEST_RUN(test_clone_from(deque_after_push_back, DequeInt()));
  SOUL_TEST_RUN(test_clone_from(DequeInt(), deque_after_push_front));
  SOUL_TEST_RUN(test_clone_from(deque_after_push_front, DequeInt()));
  SOUL_TEST_RUN(test_clone_from(DequeInt(), deque_after_push_front));
  SOUL_TEST_RUN(test_clone_from(DequeInt(), DequeInt()));

  SOUL_TEST_RUN(test_clone_from(deque_after_pop_front, deque_after_pop_back));
  SOUL_TEST_RUN(test_clone_from(deque_after_pop_back, deque_after_pop_front));
  SOUL_TEST_RUN(test_clone_from(DequeObj(), deque_after_pop_front));
  SOUL_TEST_RUN(test_clone_from(deque_after_pop_front, DequeObj()));
  SOUL_TEST_RUN(test_clone_from(DequeObj(), deque_after_pop_back));
  SOUL_TEST_RUN(test_clone_from(deque_after_pop_back, DequeObj()));
  SOUL_TEST_RUN(test_clone_from(DequeObj(), deque_after_pop_back));
  SOUL_TEST_RUN(test_clone_from(DequeObj(), DequeObj()));
}

TEST_F(TestDequeManipulationAfterManipulation, TestSwap)
{
  SOUL_TEST_RUN(test_swap(deque_after_push_back, deque_after_push_front));
  SOUL_TEST_RUN(test_swap(deque_after_push_front, deque_after_push_back));
  SOUL_TEST_RUN(test_swap(DequeInt(), deque_after_push_back));
  SOUL_TEST_RUN(test_swap(deque_after_push_back, DequeInt()));
  SOUL_TEST_RUN(test_swap(DequeInt(), deque_after_push_front));
  SOUL_TEST_RUN(test_swap(deque_after_push_front, DequeInt()));
  SOUL_TEST_RUN(test_swap(DequeInt(), deque_after_push_front));
  SOUL_TEST_RUN(test_swap(DequeInt(), DequeInt()));

  SOUL_TEST_RUN(test_swap(deque_after_pop_front, deque_after_pop_back));
  SOUL_TEST_RUN(test_swap(deque_after_pop_back, deque_after_pop_front));
  SOUL_TEST_RUN(test_swap(DequeObj(), deque_after_pop_front));
  SOUL_TEST_RUN(test_swap(deque_after_pop_front, DequeObj()));
  SOUL_TEST_RUN(test_swap(DequeObj(), deque_after_pop_back));
  SOUL_TEST_RUN(test_swap(deque_after_pop_back, DequeObj()));
  SOUL_TEST_RUN(test_swap(DequeObj(), deque_after_pop_back));
  SOUL_TEST_RUN(test_swap(DequeObj(), DequeObj()));
}

TEST_F(TestDequeManipulationAfterManipulation, TestClear)
{
  SOUL_TEST_RUN(test_deque_clear(deque_after_push_back));
  SOUL_TEST_RUN(test_deque_clear(deque_after_push_front));
  SOUL_TEST_RUN(test_deque_clear(deque_after_pop_back));
  SOUL_TEST_RUN(test_deque_clear(deque_after_pop_front));
}

TEST_F(TestDequeManipulationAfterManipulation, TestCleanup)
{
  SOUL_TEST_RUN(test_deque_cleanup(deque_after_push_back));
  SOUL_TEST_RUN(test_deque_cleanup(deque_after_push_front));
  SOUL_TEST_RUN(test_deque_cleanup(deque_after_pop_back));
  SOUL_TEST_RUN(test_deque_cleanup(deque_after_pop_front));
}

TEST_F(TestDequeManipulationAfterManipulation, TestReserve)
{
  SOUL_TEST_RUN(test_reserve(deque_after_push_back, 0));
  SOUL_TEST_RUN(test_reserve(deque_after_push_back, 10));
  SOUL_TEST_RUN(test_reserve(deque_after_push_back, deque_after_push_back.size() / 2));
  SOUL_TEST_RUN(test_reserve(deque_after_push_back, deque_after_push_back.size() * 2));

  SOUL_TEST_RUN(test_reserve(deque_after_push_front, 0));
  SOUL_TEST_RUN(test_reserve(deque_after_push_front, 10));
  SOUL_TEST_RUN(test_reserve(deque_after_push_front, deque_after_push_front.size() / 2));
  SOUL_TEST_RUN(test_reserve(deque_after_push_front, deque_after_push_front.size() * 2));

  SOUL_TEST_RUN(test_reserve(deque_after_pop_front, 0));
  SOUL_TEST_RUN(test_reserve(deque_after_pop_front, 10));
  SOUL_TEST_RUN(test_reserve(deque_after_pop_front, deque_after_pop_front.size() / 2));
  SOUL_TEST_RUN(test_reserve(deque_after_pop_front, deque_after_pop_front.size() * 2));

  SOUL_TEST_RUN(test_reserve(deque_after_pop_back, 0));
  SOUL_TEST_RUN(test_reserve(deque_after_pop_back, 10));
  SOUL_TEST_RUN(test_reserve(deque_after_pop_back, deque_after_pop_back.size() / 2));
  SOUL_TEST_RUN(test_reserve(deque_after_pop_back, deque_after_pop_back.size() * 2));
}

TEST_F(TestDequeManipulationAfterManipulation, TestShrinkToFit)
{
  SOUL_TEST_RUN(test_deque_shrink_to_fit(deque_after_push_back, deque_after_push_back.capacity()));
  SOUL_TEST_RUN(
    test_deque_shrink_to_fit(deque_after_push_back, deque_after_push_back.capacity() + 5));

  SOUL_TEST_RUN(
    test_deque_shrink_to_fit(deque_after_push_front, deque_after_push_front.capacity()));
  SOUL_TEST_RUN(
    test_deque_shrink_to_fit(deque_after_push_front, deque_after_push_front.capacity() + 5));

  SOUL_TEST_RUN(test_deque_shrink_to_fit(deque_after_pop_front, deque_after_pop_front.capacity()));
  SOUL_TEST_RUN(
    test_deque_shrink_to_fit(deque_after_pop_front, deque_after_pop_front.capacity() + 5));

  SOUL_TEST_RUN(test_deque_shrink_to_fit(deque_after_pop_back, deque_after_pop_back.capacity()));
  SOUL_TEST_RUN(
    test_deque_shrink_to_fit(deque_after_pop_back, deque_after_pop_back.capacity() + 5));
}

TEST_F(TestDequeManipulationAfterManipulation, TestPushBack)
{

  SOUL_TEST_RUN(test_deque_push_back(deque_after_push_back, 5));
  SOUL_TEST_RUN(test_deque_push_back(deque_after_push_front, 5));
  SOUL_TEST_RUN(test_deque_push_back(deque_after_pop_back, TestObject(5)));
  SOUL_TEST_RUN(test_deque_push_back(deque_after_pop_front, TestObject(5)));
}

TEST_F(TestDequeManipulationAfterManipulation, TestPushFront)
{

  SOUL_TEST_RUN(test_deque_push_front(deque_after_push_back, 5));
  SOUL_TEST_RUN(test_deque_push_front(deque_after_push_front, 5));
  SOUL_TEST_RUN(test_deque_push_front(deque_after_pop_back, TestObject(5)));
  SOUL_TEST_RUN(test_deque_push_front(deque_after_pop_front, TestObject(5)));
}

TEST_F(TestDequeManipulationAfterManipulation, TestPopBack)
{

  SOUL_TEST_RUN(test_deque_pop_back(deque_after_push_back));
  SOUL_TEST_RUN(test_deque_pop_back(deque_after_push_front));
  SOUL_TEST_RUN(test_deque_pop_back(deque_after_pop_back));
  SOUL_TEST_RUN(test_deque_pop_back(deque_after_pop_front));
}

TEST_F(TestDequeManipulationAfterManipulation, TestPopFront)
{

  SOUL_TEST_RUN(test_deque_pop_front(deque_after_push_back));
  SOUL_TEST_RUN(test_deque_pop_front(deque_after_push_front));
  SOUL_TEST_RUN(test_deque_pop_front(deque_after_pop_back));
  SOUL_TEST_RUN(test_deque_pop_front(deque_after_pop_front));
}

class TestEmptyDequeManipulationAfterManipulation : public testing::Test
{
public:
  DequeObj deque_obj_empty =
    DequeObj::From(generate_random_sequence<TestObject>(8) | soul::views::clone<TestObject>());

  DequeObj deque_obj_filled =
    DequeObj::From(generate_random_sequence<TestObject>(100) | soul::views::clone<TestObject>());

  TestEmptyDequeManipulationAfterManipulation()
  {
    while (!deque_obj_empty.empty())
    {
      deque_obj_empty.pop_front();
    }
  }
};

TEST_F(TestEmptyDequeManipulationAfterManipulation, TestClone)
{
  SOUL_TEST_RUN(test_clone(deque_obj_empty));
}

TEST_F(TestEmptyDequeManipulationAfterManipulation, TestCloneFromFilledToEmtpy)
{
  const auto expected = deque_obj_filled.clone();
  deque_obj_empty.clone_from(deque_obj_filled);
  SOUL_TEST_RUN(verify_equal(deque_obj_filled, expected));
}

TEST_F(TestEmptyDequeManipulationAfterManipulation, TestCloneFromEmptyToFilled)
{
  deque_obj_filled.clone_from(deque_obj_empty);
  SOUL_TEST_RUN(verify_equal(deque_obj_filled, deque_obj_empty));
}

TEST_F(TestEmptyDequeManipulationAfterManipulation, TestMoveConstructor)
{
  const auto deque_dst = std::move(deque_obj_empty);
  SOUL_TEST_RUN(verify_equal(deque_dst, DequeObj()));
}

TEST_F(TestEmptyDequeManipulationAfterManipulation, TestMoveFromFilledToEmtpy)
{
  const auto expected = deque_obj_filled.clone();
  deque_obj_empty     = std::move(deque_obj_filled);
  SOUL_TEST_RUN(verify_equal(deque_obj_empty, expected));
}

TEST_F(TestEmptyDequeManipulationAfterManipulation, TestMoveFromEmptyToFilled)
{
  deque_obj_filled = std::move(deque_obj_empty);
  SOUL_TEST_RUN(verify_equal(deque_obj_filled, DequeObj()));
}

TEST_F(TestEmptyDequeManipulationAfterManipulation, TestPushBack)
{
  const auto test_obj = TestObject(33);
  deque_obj_empty.push_back(test_obj.clone());
  SOUL_TEST_ASSERT_EQ(deque_obj_empty.size(), 1);
  SOUL_TEST_ASSERT_EQ(deque_obj_empty.front_ref(), test_obj);
  SOUL_TEST_ASSERT_EQ(deque_obj_empty.back_ref(), test_obj);

  SOUL_TEST_RUN(verify_equal(
    deque_obj_empty, DequeObj::From(Array{test_obj.clone()} | soul::views::clone<TestObject>())));
}

TEST_F(TestEmptyDequeManipulationAfterManipulation, TestPushFront)
{
  const auto test_obj = TestObject(33);
  deque_obj_empty.push_front(test_obj.clone());
  SOUL_TEST_ASSERT_EQ(deque_obj_empty.size(), 1);
  SOUL_TEST_ASSERT_EQ(deque_obj_empty.front_ref(), test_obj);
  SOUL_TEST_ASSERT_EQ(deque_obj_empty.back_ref(), test_obj);

  SOUL_TEST_RUN(verify_equal(
    deque_obj_empty, DequeObj::From(Array{test_obj.clone()} | soul::views::clone<TestObject>())));
}
