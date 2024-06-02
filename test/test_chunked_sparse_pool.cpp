#include <algorithm>
#include <numeric>
#include <random>
#include <xutility>

#include <gtest/gtest.h>

#include "core/array.h"
#include "core/chunked_sparse_pool.h"
#include "core/config.h"
#include "core/mutex.h"
#include "core/objops.h"
#include "core/rid.h"
#include "core/views.h"
#include "memory/allocator.h"

#include "util.h"

namespace soul
{
  auto get_default_allocator() -> memory::Allocator*
  {
    static TestAllocator test_allocator("Test default allocator"_str);
    return &test_allocator;
  }
} // namespace soul

using IntId   = soul::RID<struct int_tag>;
using PoolInt = soul::ChunkedSparsePool<int, IntId, NullMutex, 4>;

using ObjId   = soul::RID<struct obj_tag>;
using PoolObj = soul::ChunkedSparsePool<TestObject, ObjId, NullMutex, 4>;

using ListObjId   = soul::RID<struct list_obj_tag>;
using PoolListObj = soul::ChunkedSparsePool<ListTestObject, ListObjId, NullMutex, 4>;

template <typename PoolT>
void test_default_constructor()
{
  PoolT pool;
  SOUL_TEST_ASSERT_TRUE(pool.size() == 0);
  SOUL_TEST_ASSERT_TRUE(pool.is_empty());
}

TEST(TestPoolConstruction, TestDefaultConstructor)
{
  SOUL_TEST_RUN(test_default_constructor<PoolInt>());
  SOUL_TEST_RUN(test_default_constructor<PoolObj>());
  SOUL_TEST_RUN(test_default_constructor<PoolListObj>());
}

template <typename PoolT>
void test_create_and_destroy()
{
  PoolT pool;
  const auto sequence1 = generate_random_sequence<typename PoolT::value_type>(10);
  soul::Vector<typename PoolT::rid_type> rids1;
  for (const auto& item : sequence1)
  {
    const auto rid = pool.create(soul::duplicate(item));
    rids1.push_back(rid);
  }

  SOUL_TEST_ASSERT_EQ(pool.size(), sequence1.size());
  for (u64 seq_i = 0; seq_i < sequence1.size(); seq_i++)
  {
    const auto rid = rids1[seq_i];
    SOUL_TEST_ASSERT_EQ(pool[rid], sequence1[seq_i]);
    SOUL_TEST_ASSERT_EQ(pool.ref(rid), sequence1[seq_i]);
    SOUL_TEST_ASSERT_EQ(pool.cref(rid), sequence1[seq_i]);
    SOUL_TEST_ASSERT_TRUE(pool.is_alive(rid));
  }

  const auto middle_index = sequence1.size() / 2;
  for (u64 seq_i = middle_index; seq_i < sequence1.size(); seq_i++)
  {
    const auto rid = rids1[seq_i];
    pool.destroy(rid);
    SOUL_TEST_ASSERT_FALSE(pool.is_alive(rid));
  }

  const auto sequence2 = generate_random_sequence<typename PoolT::value_type>(10);
  soul::Vector<typename PoolT::rid_type> rids2;
  for (const auto& item : sequence2)
  {
    const auto rid = pool.create(soul::duplicate(item));
    rids2.push_back(rid);
  }
  for (u64 seq_i = 0; seq_i < sequence2.size(); seq_i++)
  {
    const auto rid = rids2[seq_i];
    SOUL_TEST_ASSERT_EQ(pool[rid], sequence2[seq_i]);
    SOUL_TEST_ASSERT_EQ(pool.ref(rid), sequence2[seq_i]);
    SOUL_TEST_ASSERT_EQ(pool.cref(rid), sequence2[seq_i]);
    SOUL_TEST_ASSERT_TRUE(pool.is_alive(rid));
  }

  for (u64 seq_i = 0; seq_i < middle_index; seq_i++)
  {
    const auto rid = rids1[seq_i];
    pool.destroy(rid);
    SOUL_TEST_ASSERT_FALSE(pool.is_alive(rid));
  }
  for (u64 seq_i = 0; seq_i < sequence2.size(); seq_i++)
  {
    const auto rid = rids2[seq_i];
    pool.destroy(rid);
    SOUL_TEST_ASSERT_FALSE(pool.is_alive(rid));
  }
  SOUL_TEST_ASSERT_EQ(pool.size(), 0);
  SOUL_TEST_ASSERT_TRUE(pool.is_empty());
}

TEST(TestPoolCreateAndDestroy, TestPoolCreateAndDestroy)
{
  test_create_and_destroy<PoolInt>();
  test_create_and_destroy<PoolObj>();
  test_create_and_destroy<PoolListObj>();
}

template <typename PoolT>
auto fill_pool_randomly(PoolT* pool, u64 object_count) -> soul::Vector<typename PoolT::rid_type>
{
  const auto sequence = generate_random_sequence<typename PoolT::value_type>(object_count);
  soul::Vector<typename PoolT::rid_type> rids;
  for (const auto& item : sequence)
  {
    const auto rid = pool->create(soul::duplicate(item));
    rids.push_back(rid);
  }
  return rids;
}

template <typename PoolT>
void test_clear()
{
  // test clearing empty pool
  {
    PoolT pool;
    pool.clear();
    SOUL_TEST_ASSERT_EQ(pool.size(), 0);
    SOUL_TEST_ASSERT_TRUE(pool.is_empty());
  }

  // test clearing pool that have create object only
  // never destroy object
  {
    PoolT pool;
    fill_pool_randomly(&pool, 10);
    const auto old_capacity = pool.capacity();
    pool.clear();
    SOUL_TEST_ASSERT_EQ(pool.size(), 0);
    SOUL_TEST_ASSERT_TRUE(pool.is_empty());
    SOUL_TEST_ASSERT_EQ(pool.capacity(), old_capacity);
  }

  // test clearing pool that have create and destroy object
  {
    PoolT pool;
    const auto rids = fill_pool_randomly(&pool, 10);
    pool.destroy(rids[5]);
    pool.destroy(rids[0]);
    pool.destroy(rids[9]);
    const auto old_capacity = pool.capacity();
    pool.clear();
    SOUL_TEST_ASSERT_EQ(pool.size(), 0);
    SOUL_TEST_ASSERT_TRUE(pool.is_empty());
    SOUL_TEST_ASSERT_EQ(pool.capacity(), old_capacity);
  }
}

TEST(TestPoolClear, TestPoolClear)
{
  test_clear<PoolInt>();
  test_clear<PoolObj>();
  test_clear<PoolListObj>();
}

template <typename PoolT>
void test_cleanup()
{
  // test cleanuping empty pool
  {
    PoolT pool;
    pool.cleanup();
    SOUL_TEST_ASSERT_EQ(pool.size(), 0);
    SOUL_TEST_ASSERT_TRUE(pool.is_empty());
    SOUL_TEST_ASSERT_EQ(pool.capacity(), 0);
  }

  // test cleanuping pool that have create object only
  // never destroy object
  {
    PoolT pool;
    fill_pool_randomly(&pool, 10);
    pool.cleanup();
    SOUL_TEST_ASSERT_EQ(pool.size(), 0);
    SOUL_TEST_ASSERT_TRUE(pool.is_empty());
    SOUL_TEST_ASSERT_EQ(pool.capacity(), 0);
  }

  // test cleanuping pool that have create and destroy object
  {
    PoolT pool;
    const auto rids = fill_pool_randomly(&pool, 10);
    pool.destroy(rids[5]);
    pool.destroy(rids[0]);
    pool.destroy(rids[9]);
    pool.cleanup();
    SOUL_TEST_ASSERT_EQ(pool.size(), 0);
    SOUL_TEST_ASSERT_TRUE(pool.is_empty());
    SOUL_TEST_ASSERT_EQ(pool.capacity(), 0);
  }
}
