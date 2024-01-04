#include <algorithm>
#include <numeric>
#include <random>
#include <xutility>

#include <gtest/gtest.h>

#include "core/config.h"
#include "core/soa_vector.h"
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

using TestStructure = soul::Tuple<u64, TestObject, i32>;
using TestSoaVector = soul::SoaVector<TestStructure>;

TEST(TestSoaVector, TestSoaVector)
{
  {
    TestSoaVector soa_vector;
    SOUL_TEST_ASSERT_EQ(soa_vector.size(), 0);
    SOUL_TEST_ASSERT_TRUE(soa_vector.empty());

    static constexpr auto PUSH_BACK_COUNT = 100;
    auto u64_sequence                     = generate_random_sequence<u64>(PUSH_BACK_COUNT);
    auto testobj_sequence                 = generate_random_sequence<TestObject>(PUSH_BACK_COUNT);
    auto i32_sequence                     = generate_random_sequence<i32>(PUSH_BACK_COUNT);

    for (auto index = 0; index < PUSH_BACK_COUNT; index++)
    {
      soa_vector.push_back(
        u64_sequence[index], testobj_sequence[index].clone(), i32_sequence[index]);
    }

    SOUL_TEST_ASSERT_EQ(soa_vector.size(), PUSH_BACK_COUNT);
    SOUL_TEST_ASSERT_FALSE(soa_vector.empty());
    for (auto index = 0; index < PUSH_BACK_COUNT; index++)
    {
      SOUL_TEST_ASSERT_EQ(soa_vector.ref<0>(index), u64_sequence[index]);
      SOUL_TEST_ASSERT_EQ(soa_vector.ref<1>(index), testobj_sequence[index]);
      SOUL_TEST_ASSERT_EQ(soa_vector.ref<2>(index), i32_sequence[index]);
    }

    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(soa_vector.span<0>(), u64_sequence));
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(soa_vector.span<1>(), testobj_sequence));
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(soa_vector.span<2>(), i32_sequence));

    u64_sequence[55]     = 100;
    testobj_sequence[55] = TestObject(100);
    i32_sequence[55]     = 100;
    soa_vector.for_each(
      55,
      [&]<usize IndexV>(auto& val)
      {
        if constexpr (IndexV == 0)
        {
          val = u64_sequence[55];
        } else if constexpr (IndexV == 1)
        {
          val = testobj_sequence[55].clone();
        } else
        {
          val = i32_sequence[55];
        }
      });
    SOUL_TEST_ASSERT_EQ(soa_vector.size(), PUSH_BACK_COUNT);
    SOUL_TEST_ASSERT_FALSE(soa_vector.empty());
    for (auto index = 0; index < PUSH_BACK_COUNT; index++)
    {
      SOUL_TEST_ASSERT_EQ(soa_vector.ref<0>(index), u64_sequence[index]) << "Index : " << index;
      SOUL_TEST_ASSERT_EQ(soa_vector.ref<1>(index), testobj_sequence[index]);
      SOUL_TEST_ASSERT_EQ(soa_vector.ref<2>(index), i32_sequence[index]);
    }

    TestSoaVector soa_vector_from_move = std::move(soa_vector);
    SOUL_TEST_ASSERT_EQ(soa_vector_from_move.size(), PUSH_BACK_COUNT);
    SOUL_TEST_ASSERT_FALSE(soa_vector_from_move.empty());
    for (auto index = 0; index < PUSH_BACK_COUNT; index++)
    {
      SOUL_TEST_ASSERT_EQ(soa_vector_from_move.ref<0>(index), u64_sequence[index]);
      SOUL_TEST_ASSERT_EQ(soa_vector_from_move.ref<1>(index), testobj_sequence[index]);
      SOUL_TEST_ASSERT_EQ(soa_vector_from_move.ref<2>(index), i32_sequence[index]);
    }

    soa_vector_from_move.pop_back();
    SOUL_TEST_ASSERT_EQ(soa_vector_from_move.size(), PUSH_BACK_COUNT - 1);
    SOUL_TEST_ASSERT_FALSE(soa_vector_from_move.empty());
    for (auto index = 0; index < PUSH_BACK_COUNT - 1; index++)
    {
      SOUL_TEST_ASSERT_EQ(soa_vector_from_move.ref<0>(index), u64_sequence[index]);
      SOUL_TEST_ASSERT_EQ(soa_vector_from_move.ref<1>(index), testobj_sequence[index]);
      SOUL_TEST_ASSERT_EQ(soa_vector_from_move.ref<2>(index), i32_sequence[index]);
    }

    u64_sequence[55]     = u64_sequence[PUSH_BACK_COUNT - 2];
    testobj_sequence[55] = testobj_sequence[PUSH_BACK_COUNT - 2].clone();
    i32_sequence[55]     = i32_sequence[PUSH_BACK_COUNT - 2];
    soa_vector_from_move.remove(55);
    SOUL_TEST_ASSERT_EQ(soa_vector_from_move.size(), PUSH_BACK_COUNT - 2);
    SOUL_TEST_ASSERT_FALSE(soa_vector_from_move.empty());
    for (auto index = 0; index < PUSH_BACK_COUNT - 2; index++)
    {
      SOUL_TEST_ASSERT_EQ(soa_vector_from_move.ref<0>(index), u64_sequence[index]);
      SOUL_TEST_ASSERT_EQ(soa_vector_from_move.ref<1>(index), testobj_sequence[index]);
      SOUL_TEST_ASSERT_EQ(soa_vector_from_move.ref<2>(index), i32_sequence[index]);
    }

    soa_vector_from_move.clear();
    SOUL_TEST_ASSERT_EQ(soa_vector_from_move.size(), 0);
    SOUL_TEST_ASSERT_TRUE(soa_vector_from_move.empty());
  }
}
