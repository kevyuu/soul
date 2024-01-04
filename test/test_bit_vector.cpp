#include <random>
#include <vector>

#include "core/bit_vector.h"
#include "core/config.h"
#include "core/util.h"

#include "util.h"

namespace soul
{
  auto get_default_allocator() -> memory::Allocator*
  {
    static TestAllocator test_allocator("Test default allocator");
    return &test_allocator;
  }
} // namespace soul

auto generate_random_bool_vector(const usize size) -> std::vector<b8>
{
  std::random_device random_device;
  std::mt19937 random_engine(random_device());
  std::uniform_int_distribution<int> dist(0, 1);

  std::vector<b8> result;
  result.reserve(size);
  for (usize i = 0; i < size; i++)
  {
    result.push_back(static_cast<b8>(dist(random_engine)));
  }
  return result;
}

template <soul::ts_bit_block T>
auto get_vector_from_bit_vector(const soul::BitVector<T>& bit_vector) -> std::vector<b8>
{
  std::vector<b8> result;
  for (usize i = 0; i < bit_vector.size(); i++)
  {
    result.push_back(bit_vector[i]);
  }
  return result;
}

template <soul::ts_bit_block BlockType>
auto verify_sequence(
  const soul::BitVector<BlockType>& bit_vector, const std::vector<b8>& src_vector) -> void
{
  SOUL_TEST_ASSERT_EQ(bit_vector.size(), src_vector.size());
  SOUL_TEST_ASSERT_EQ(bit_vector.empty(), src_vector.empty());
  if (!src_vector.empty())
  {
    SOUL_TEST_ASSERT_EQ(bit_vector.front(), src_vector.front());
    SOUL_TEST_ASSERT_EQ(bit_vector.back(), src_vector.back());
  }
  for (usize i = 0; i < src_vector.size(); i++)
  {
    SOUL_TEST_ASSERT_EQ(bit_vector[i], src_vector[i]) << ",i : " << i;
    SOUL_TEST_ASSERT_EQ(bit_vector.test(i, false), src_vector[i]) << ",i : " << i;
    SOUL_TEST_ASSERT_EQ(bit_vector.test(i, true), src_vector[i]) << ",i : " << i;
  }
}

template <soul::ts_bit_block BlockType>
auto test_constructor() -> void
{
  const soul::BitVector<BlockType> bit_vector;
  SOUL_TEST_ASSERT_TRUE(bit_vector.empty());
  SOUL_TEST_ASSERT_EQ(bit_vector.size(), 0);
  SOUL_TEST_ASSERT_EQ(bit_vector.capacity(), 0);
}

TEST(TestBitVectorConstruction, TestDefaultConstructor)
{
  SOUL_TEST_RUN(test_constructor<u8>());
  SOUL_TEST_RUN(test_constructor<u16>());
  SOUL_TEST_RUN(test_constructor<u32>());
  SOUL_TEST_RUN(test_constructor<u64>());
}

template <soul::ts_bit_block BlockType>
auto test_construction_init_fill_n(const usize size, const b8 val) -> void
{
  const auto bit_vector = soul::BitVector<BlockType>::FillN(size, val);
  SOUL_TEST_ASSERT_EQ(bit_vector.size(), size);
  SOUL_TEST_ASSERT_EQ(bit_vector.empty(), size == 0);
  for (usize i = 0; i < size; i++)
  {
    SOUL_TEST_ASSERT_EQ(bit_vector[i], val);
    SOUL_TEST_ASSERT_EQ(bit_vector.test(i, false), val);
    SOUL_TEST_ASSERT_EQ(bit_vector.test(i, true), val);
  }
  if (size != 0)
  {
    SOUL_TEST_ASSERT_EQ(bit_vector.front(), val);
    SOUL_TEST_ASSERT_EQ(bit_vector.back(), val);
  }
}

TEST(TestBitVectorConstruction, TestConstructionInitFullN)
{
  SOUL_TEST_RUN(test_construction_init_fill_n<u8>(0, false));
  SOUL_TEST_RUN(test_construction_init_fill_n<u8>(8, true));
  SOUL_TEST_RUN(test_construction_init_fill_n<u8>(1, false));
  SOUL_TEST_RUN(test_construction_init_fill_n<u8>(20, true));

  SOUL_TEST_RUN(test_construction_init_fill_n<u64>(0, true));
  SOUL_TEST_RUN(test_construction_init_fill_n<u64>(64, false));
  SOUL_TEST_RUN(test_construction_init_fill_n<u64>(1, true));
  SOUL_TEST_RUN(test_construction_init_fill_n<u64>(130, false));
}

template <soul::ts_bit_block BlockType>
auto test_construction_with_capacity(const usize capacity) -> void
{
  const auto bit_vector = soul::BitVector<BlockType>::WithCapacity(capacity);
  SOUL_TEST_ASSERT_EQ(bit_vector.size(), 0);
  SOUL_TEST_ASSERT_TRUE(bit_vector.empty());
}

TEST(TestBitVectorConstruction, TestConstructionWithCapacity)
{
  SOUL_TEST_RUN(test_construction_with_capacity<u8>(0));
  SOUL_TEST_RUN(test_construction_with_capacity<u8>(4));
  SOUL_TEST_RUN(test_construction_with_capacity<u8>(8));
  SOUL_TEST_RUN(test_construction_with_capacity<u8>(100));

  SOUL_TEST_RUN(test_construction_with_capacity<u64>(0));
  SOUL_TEST_RUN(test_construction_with_capacity<u64>(64));
  SOUL_TEST_RUN(test_construction_with_capacity<u64>(1));
  SOUL_TEST_RUN(test_construction_with_capacity<u64>(130));
}

template <soul::ts_bit_block BlockType>
auto test_construction_from_range(const usize size) -> void
{
  SOUL_TEST_ASSERT_NE(size, 0);
  const std::vector<b8> random_bool_vec = generate_random_bool_vector(size);

  const auto bit_vector = soul::BitVector<BlockType>::From(random_bool_vec);
  verify_sequence(bit_vector, random_bool_vec);
}

TEST(TestBitVectorConstruction, TestConstructionFromRange)
{
  SOUL_TEST_RUN(test_construction_from_range<u8>(8));
  SOUL_TEST_RUN(test_construction_from_range<u8>(1));
  SOUL_TEST_RUN(test_construction_from_range<u8>(20));

  SOUL_TEST_RUN(test_construction_from_range<u64>(64));
  SOUL_TEST_RUN(test_construction_from_range<u64>(1));
  SOUL_TEST_RUN(test_construction_from_range<u64>(130));
}

template <soul::ts_bit_block BlockType>
auto test_clone(const usize size) -> void
{
  const std::vector<b8> random_bool_vec = generate_random_bool_vector(size);
  const auto src_bit_vector             = soul::BitVector<BlockType>::From(random_bool_vec);
  const soul::BitVector<BlockType> test_bit_vector = src_bit_vector.clone();
  verify_sequence(test_bit_vector, random_bool_vec);
}

TEST(TestBitVectorConstruction, TestClone)
{
  SOUL_TEST_RUN(test_clone<u8>(0));
  SOUL_TEST_RUN(test_clone<u8>(8));
  SOUL_TEST_RUN(test_clone<u8>(1));
  SOUL_TEST_RUN(test_clone<u8>(20));

  SOUL_TEST_RUN(test_clone<u64>(0));
  SOUL_TEST_RUN(test_clone<u64>(64));
  SOUL_TEST_RUN(test_clone<u64>(1));
  SOUL_TEST_RUN(test_clone<u64>(130));
}

template <soul::ts_bit_block BlockType>
auto test_move_constructor(const usize size) -> void
{
  const std::vector<b8> random_bool_vec = generate_random_bool_vector(size);
  auto src_bit_vector                   = soul::BitVector<BlockType>::From(random_bool_vec);
  const soul::BitVector<BlockType> test_bit_vector = std::move(src_bit_vector);
  verify_sequence(test_bit_vector, random_bool_vec);
}

TEST(TestBitVectorConstruction, TestMoveConstructor)
{
  SOUL_TEST_RUN(test_move_constructor<u8>(0));
  SOUL_TEST_RUN(test_move_constructor<u8>(8));
  SOUL_TEST_RUN(test_move_constructor<u8>(1));
  SOUL_TEST_RUN(test_move_constructor<u8>(20));

  SOUL_TEST_RUN(test_move_constructor<u64>(0));
  SOUL_TEST_RUN(test_move_constructor<u64>(64));
  SOUL_TEST_RUN(test_move_constructor<u64>(1));
  SOUL_TEST_RUN(test_move_constructor<u64>(130));
}

class TestBitVectorManipulation : public testing::Test
{
public:
  static constexpr usize RANDOM_BOOL_VECTOR_SIZE = 130;

  TestBitVectorManipulation()
      : sources_vec(generate_random_bool_vector(RANDOM_BOOL_VECTOR_SIZE)),
        u8_filled_bit_vector(soul::BitVector<u8>::From(sources_vec)),
        u32_filled_bit_vector(soul::BitVector<u32>::From(sources_vec)),
        u64_filled_bit_vector(soul::BitVector<u64>::FillN(RANDOM_BOOL_VECTOR_SIZE, true))
  {
  }

  std::vector<b8> sources_vec;
  soul::BitVector<> empty_bit_vector;

  soul::BitVector<u8> u8_filled_bit_vector;
  soul::BitVector<u32> u32_filled_bit_vector;
  soul::BitVector<u64> u64_filled_bit_vector;
};

TEST_F(TestBitVectorManipulation, TestBitVectorResize)
{
  auto test_resize =
    []<soul::ts_bit_block BlockType>(const soul::BitVector<BlockType>& bit_vector, const usize size)
  {
    auto test_vector     = bit_vector.clone();
    auto expected_vector = get_vector_from_bit_vector(bit_vector);
    expected_vector.resize(size);
    test_vector.resize(size);
    verify_sequence(test_vector, expected_vector);
  };

  SOUL_TEST_RUN(test_resize(empty_bit_vector, 0));
  SOUL_TEST_RUN(test_resize(empty_bit_vector, 1));
  SOUL_TEST_RUN(test_resize(empty_bit_vector, 130));

  SOUL_TEST_RUN(test_resize(u8_filled_bit_vector, 0));
  SOUL_TEST_RUN(test_resize(u8_filled_bit_vector, 1));
  SOUL_TEST_RUN(test_resize(u8_filled_bit_vector, u8_filled_bit_vector.size()));
  SOUL_TEST_RUN(test_resize(u8_filled_bit_vector, u8_filled_bit_vector.size() + 9));
  SOUL_TEST_RUN(test_resize(u8_filled_bit_vector, u8_filled_bit_vector.size() - 9));

  SOUL_TEST_RUN(test_resize(u32_filled_bit_vector, 0));
  SOUL_TEST_RUN(test_resize(u32_filled_bit_vector, 1));
  SOUL_TEST_RUN(test_resize(u32_filled_bit_vector, u32_filled_bit_vector.size()));
  SOUL_TEST_RUN(test_resize(u32_filled_bit_vector, u32_filled_bit_vector.size() + 9));
  SOUL_TEST_RUN(test_resize(u32_filled_bit_vector, u32_filled_bit_vector.size() + 90));
  SOUL_TEST_RUN(test_resize(u32_filled_bit_vector, u32_filled_bit_vector.size() - 30));

  SOUL_TEST_RUN(test_resize(u64_filled_bit_vector, 0));
  SOUL_TEST_RUN(test_resize(u64_filled_bit_vector, 1));
  SOUL_TEST_RUN(test_resize(u64_filled_bit_vector, u64_filled_bit_vector.size()));
  SOUL_TEST_RUN(test_resize(u64_filled_bit_vector, u64_filled_bit_vector.size() + 260));
  SOUL_TEST_RUN(test_resize(u64_filled_bit_vector, u64_filled_bit_vector.size() - 60));
}

TEST_F(TestBitVectorManipulation, TestBitVectorReserve)
{
  auto test_reserve = []<soul::ts_bit_block BlockType>(
                        const soul::BitVector<BlockType>& sample_vector, const usize new_capacity)
  {
    auto test_vector     = sample_vector.clone();
    auto expected_vector = get_vector_from_bit_vector(test_vector);
    expected_vector.reserve(new_capacity);

    test_vector.reserve(new_capacity);
    SOUL_TEST_ASSERT_GE(test_vector.capacity(), new_capacity);
    verify_sequence(test_vector, expected_vector);
  };

  SOUL_TEST_RUN(test_reserve(empty_bit_vector, 0));
  SOUL_TEST_RUN(test_reserve(empty_bit_vector, 1));
  SOUL_TEST_RUN(test_reserve(empty_bit_vector, 130));

  SOUL_TEST_RUN(test_reserve(u8_filled_bit_vector, 0));
  SOUL_TEST_RUN(test_reserve(u8_filled_bit_vector, 1));
  SOUL_TEST_RUN(test_reserve(u8_filled_bit_vector, u8_filled_bit_vector.size()));
  SOUL_TEST_RUN(test_reserve(u8_filled_bit_vector, u8_filled_bit_vector.size() + 9));
  SOUL_TEST_RUN(test_reserve(u8_filled_bit_vector, u8_filled_bit_vector.size() - 9));

  SOUL_TEST_RUN(test_reserve(u32_filled_bit_vector, 0));
  SOUL_TEST_RUN(test_reserve(u32_filled_bit_vector, 1));
  SOUL_TEST_RUN(test_reserve(u32_filled_bit_vector, u32_filled_bit_vector.size()));
  SOUL_TEST_RUN(test_reserve(u32_filled_bit_vector, u32_filled_bit_vector.size() + 9));
  SOUL_TEST_RUN(test_reserve(u32_filled_bit_vector, u32_filled_bit_vector.size() + 90));
  SOUL_TEST_RUN(test_reserve(u32_filled_bit_vector, u32_filled_bit_vector.size() - 30));

  SOUL_TEST_RUN(test_reserve(u64_filled_bit_vector, 0));
  SOUL_TEST_RUN(test_reserve(u64_filled_bit_vector, 1));
  SOUL_TEST_RUN(test_reserve(u64_filled_bit_vector, u64_filled_bit_vector.size()));
  SOUL_TEST_RUN(test_reserve(u64_filled_bit_vector, u64_filled_bit_vector.size() + 260));
  SOUL_TEST_RUN(test_reserve(u64_filled_bit_vector, u64_filled_bit_vector.size() - 60));
}

TEST_F(TestBitVectorManipulation, TestBitVectorClear)
{
  auto test_clear =
    []<soul::ts_bit_block BlockType>(const soul::BitVector<BlockType>& sample_vector)
  {
    auto bit_vector = sample_vector.clone();
    bit_vector.clear();
    verify_sequence(bit_vector, std::vector<b8>());
  };

  SOUL_TEST_RUN(test_clear(empty_bit_vector));
  SOUL_TEST_RUN(test_clear(u8_filled_bit_vector));
  SOUL_TEST_RUN(test_clear(u32_filled_bit_vector));
  SOUL_TEST_RUN(test_clear(u64_filled_bit_vector));
}

TEST_F(TestBitVectorManipulation, TestBitVectorCleanup)
{
  auto test_cleanup =
    []<soul::ts_bit_block BlockType>(const soul::BitVector<BlockType>& sample_vector)
  {
    auto test_vector = sample_vector.clone();
    test_vector.cleanup();
    verify_sequence(test_vector, std::vector<b8>());
    SOUL_TEST_ASSERT_EQ(test_vector.capacity(), 0);
  };

  SOUL_TEST_RUN(test_cleanup(empty_bit_vector));
  SOUL_TEST_RUN(test_cleanup(u8_filled_bit_vector));
  SOUL_TEST_RUN(test_cleanup(u32_filled_bit_vector));
  SOUL_TEST_RUN(test_cleanup(u64_filled_bit_vector));
}

TEST_F(TestBitVectorManipulation, TestBitVectorPushBack)
{
  auto test_push_back =
    []<soul::ts_bit_block BlockType>(const soul::BitVector<BlockType>& sample_vector, const b8 val)
  {
    auto test_vector     = sample_vector.clone();
    auto expected_vector = get_vector_from_bit_vector(test_vector);
    expected_vector.push_back(false);
    soul::BitRef bit_ref = test_vector.push_back();
    SOUL_TEST_ASSERT_EQ(bit_ref, false);
    verify_sequence(test_vector, expected_vector);
    expected_vector.back() = val;
    bit_ref                = val;
    verify_sequence(test_vector, expected_vector);
    SOUL_TEST_ASSERT_EQ(bit_ref, val);
  };

  SOUL_TEST_RUN(test_push_back(empty_bit_vector, true));
  SOUL_TEST_RUN(test_push_back(empty_bit_vector, false));

  SOUL_TEST_RUN(test_push_back(u8_filled_bit_vector, true));
  SOUL_TEST_RUN(test_push_back(u8_filled_bit_vector, false));

  SOUL_TEST_RUN(test_push_back(u32_filled_bit_vector, true));
  SOUL_TEST_RUN(test_push_back(u32_filled_bit_vector, false));

  SOUL_TEST_RUN(test_push_back(u64_filled_bit_vector, true));
  SOUL_TEST_RUN(test_push_back(u64_filled_bit_vector, false));

  auto test_push_back_with_val =
    []<soul::ts_bit_block BlockType>(const soul::BitVector<BlockType>& sample_vector, const b8 val)
  {
    auto test_vector     = sample_vector.clone();
    auto expected_vector = get_vector_from_bit_vector(test_vector);
    expected_vector.push_back(val);

    test_vector.push_back(val);
    verify_sequence(test_vector, expected_vector);
  };

  SOUL_TEST_RUN(test_push_back_with_val(empty_bit_vector, true));
  SOUL_TEST_RUN(test_push_back_with_val(empty_bit_vector, false));

  SOUL_TEST_RUN(test_push_back_with_val(u8_filled_bit_vector, true));
  SOUL_TEST_RUN(test_push_back_with_val(u8_filled_bit_vector, false));

  SOUL_TEST_RUN(test_push_back_with_val(u32_filled_bit_vector, true));
  SOUL_TEST_RUN(test_push_back_with_val(u32_filled_bit_vector, false));

  SOUL_TEST_RUN(test_push_back_with_val(u64_filled_bit_vector, true));
  SOUL_TEST_RUN(test_push_back_with_val(u64_filled_bit_vector, false));
}

TEST_F(TestBitVectorManipulation, TestBitVectorPopBack)
{
  auto test_pop_back =
    []<soul::ts_bit_block BlockType>(const soul::BitVector<BlockType>& sample_vector)
  {
    auto test_vector     = sample_vector.clone();
    auto expected_vector = get_vector_from_bit_vector(test_vector);
    expected_vector.pop_back();
    test_vector.pop_back();
    verify_sequence(test_vector, expected_vector);
  };

  SOUL_TEST_RUN(test_pop_back(u8_filled_bit_vector));
  SOUL_TEST_RUN(test_pop_back(u32_filled_bit_vector));
  SOUL_TEST_RUN(test_pop_back(u64_filled_bit_vector));

  auto test_pop_back_with_count =
    []<soul::ts_bit_block BlockType>(
      const soul::BitVector<BlockType>& sample_vector, const usize size)
  {
    auto test_vector     = sample_vector.clone();
    auto expected_vector = get_vector_from_bit_vector(test_vector);
    for (usize i = 0; i < size; ++i)
    {
      expected_vector.pop_back();
    }
    test_vector.pop_back(size);
    verify_sequence(test_vector, expected_vector);
  };

  SOUL_TEST_RUN(test_pop_back_with_count(u8_filled_bit_vector, 1));
  SOUL_TEST_RUN(test_pop_back_with_count(u8_filled_bit_vector, 0));
  SOUL_TEST_RUN(test_pop_back_with_count(u32_filled_bit_vector, u32_filled_bit_vector.size() / 2));
  SOUL_TEST_RUN(test_pop_back_with_count(u64_filled_bit_vector, 64));
}

TEST_F(TestBitVectorManipulation, TestBitVectorSet)
{
  auto test_set_with_index =
    []<soul::ts_bit_block BlockType>(
      const soul::BitVector<BlockType>& sample_vector, const usize index, const b8 val)
  {
    auto test_vector     = sample_vector.clone();
    auto expected_vector = get_vector_from_bit_vector(test_vector);
    if (expected_vector.size() <= index)
    {
      expected_vector.resize(index + 1);
    }
    expected_vector[index] = val;

    test_vector.set(index, val);
    verify_sequence(test_vector, expected_vector);
  };

  SOUL_TEST_RUN(test_set_with_index(empty_bit_vector, 0, true));
  SOUL_TEST_RUN(test_set_with_index(empty_bit_vector, 7, true));

  SOUL_TEST_RUN(test_set_with_index(u8_filled_bit_vector, 5, true));
  SOUL_TEST_RUN(
    test_set_with_index(u8_filled_bit_vector, u8_filled_bit_vector.capacity() + 10, false));
  SOUL_TEST_RUN(
    test_set_with_index(u8_filled_bit_vector, u8_filled_bit_vector.capacity() + 10, true));

  SOUL_TEST_RUN(test_set_with_index(u32_filled_bit_vector, 5, true));
  SOUL_TEST_RUN(
    test_set_with_index(u32_filled_bit_vector, u32_filled_bit_vector.capacity() + 10, false));
  SOUL_TEST_RUN(
    test_set_with_index(u32_filled_bit_vector, u32_filled_bit_vector.capacity() + 10, true));

  SOUL_TEST_RUN(test_set_with_index(u64_filled_bit_vector, 5, true));
  SOUL_TEST_RUN(
    test_set_with_index(u64_filled_bit_vector, u64_filled_bit_vector.capacity() + 10, false));
  SOUL_TEST_RUN(
    test_set_with_index(u64_filled_bit_vector, u64_filled_bit_vector.capacity() + 10, true));

  auto test_set = []<soul::ts_bit_block BlockType>(const soul::BitVector<BlockType>& sample_vector)
  {
    auto test_vector = sample_vector.clone();
    test_vector.set();
    verify_sequence(test_vector, std::vector<b8>(test_vector.size(), true));
  };

  SOUL_TEST_RUN(test_set(empty_bit_vector));
  SOUL_TEST_RUN(test_set(u8_filled_bit_vector));
  SOUL_TEST_RUN(test_set(u32_filled_bit_vector));
  SOUL_TEST_RUN(test_set(u64_filled_bit_vector));
}

TEST_F(TestBitVectorManipulation, TestBitVectorReset)
{
  auto test_reset =
    []<soul::ts_bit_block BlockType>(const soul::BitVector<BlockType>& sample_vector)
  {
    auto test_vector = sample_vector.clone();
    test_vector.reset();
    verify_sequence(test_vector, std::vector<b8>(test_vector.size(), false));
  };

  SOUL_TEST_RUN(test_reset(empty_bit_vector));
  SOUL_TEST_RUN(test_reset(u8_filled_bit_vector));
  SOUL_TEST_RUN(test_reset(u32_filled_bit_vector));
  SOUL_TEST_RUN(test_reset(u64_filled_bit_vector));
}

TEST_F(TestBitVectorManipulation, TestBitRef)
{
  auto test_bit_ref_and =
    []<soul::ts_bit_block BlockType>(
      const soul::BitVector<BlockType>& sample_vector, const usize idx, const b8 val)
  {
    auto test_vector     = sample_vector.clone();
    auto expected_vector = get_vector_from_bit_vector(test_vector);
    expected_vector[idx] = expected_vector[idx] && val;
    test_vector[idx] &= val;
    verify_sequence(test_vector, expected_vector);
  };

  SOUL_TEST_RUN(test_bit_ref_and(u8_filled_bit_vector, 5, true));
  SOUL_TEST_RUN(test_bit_ref_and(u8_filled_bit_vector, 5, false));
  SOUL_TEST_RUN(test_bit_ref_and(u8_filled_bit_vector, 0, true));
  SOUL_TEST_RUN(test_bit_ref_and(u8_filled_bit_vector, 0, false));
  SOUL_TEST_RUN(test_bit_ref_and(u8_filled_bit_vector, u8_filled_bit_vector.size() - 1, true));
  SOUL_TEST_RUN(test_bit_ref_and(u8_filled_bit_vector, u8_filled_bit_vector.size() - 1, false));

  auto test_bit_ref_or =
    []<soul::ts_bit_block BlockType>(
      const soul::BitVector<BlockType>& sample_vector, const usize idx, const b8 val)
  {
    auto test_vector     = sample_vector.clone();
    auto expected_vector = get_vector_from_bit_vector(test_vector);
    expected_vector[idx] = expected_vector[idx] || val;
    test_vector[idx] |= val;
    verify_sequence(test_vector, expected_vector);
  };

  SOUL_TEST_RUN(test_bit_ref_or(u8_filled_bit_vector, 5, true));
  SOUL_TEST_RUN(test_bit_ref_or(u8_filled_bit_vector, 5, false));
  SOUL_TEST_RUN(test_bit_ref_or(u8_filled_bit_vector, 0, true));
  SOUL_TEST_RUN(test_bit_ref_or(u8_filled_bit_vector, 0, false));
  SOUL_TEST_RUN(test_bit_ref_or(u8_filled_bit_vector, u8_filled_bit_vector.size() - 1, true));
  SOUL_TEST_RUN(test_bit_ref_or(u8_filled_bit_vector, u8_filled_bit_vector.size() - 1, false));

  auto test_bit_ref_xor =
    []<soul::ts_bit_block BlockType>(
      const soul::BitVector<BlockType>& sample_vector, const usize idx, const b8 val)
  {
    auto test_vector     = sample_vector.clone();
    auto expected_vector = get_vector_from_bit_vector(test_vector);
    expected_vector[idx] = (expected_vector[idx] != val);
    test_vector[idx] ^= val;
    verify_sequence(test_vector, expected_vector);
  };

  SOUL_TEST_RUN(test_bit_ref_xor(u8_filled_bit_vector, 5, true));
  SOUL_TEST_RUN(test_bit_ref_xor(u8_filled_bit_vector, 5, false));
  SOUL_TEST_RUN(test_bit_ref_xor(u8_filled_bit_vector, 0, true));
  SOUL_TEST_RUN(test_bit_ref_xor(u8_filled_bit_vector, 0, false));
  SOUL_TEST_RUN(test_bit_ref_xor(u8_filled_bit_vector, u8_filled_bit_vector.size() - 1, true));
  SOUL_TEST_RUN(test_bit_ref_xor(u8_filled_bit_vector, u8_filled_bit_vector.size() - 1, false));

  auto test_bit_flip = []<soul::ts_bit_block BlockType>(
                         const soul::BitVector<BlockType>& sample_vector, const usize idx)
  {
    auto test_vector     = sample_vector.clone();
    auto expected_vector = get_vector_from_bit_vector(test_vector);
    expected_vector[idx] = !expected_vector[idx];
    const b8 negate_val  = ~test_vector[idx];
    test_vector[idx].flip();
    verify_sequence(test_vector, expected_vector);
    SOUL_TEST_ASSERT_EQ(negate_val, expected_vector[idx]);
  };

  SOUL_TEST_RUN(test_bit_flip(u8_filled_bit_vector, 5));
  SOUL_TEST_RUN(test_bit_flip(u8_filled_bit_vector, 0));
  SOUL_TEST_RUN(test_bit_flip(u8_filled_bit_vector, u8_filled_bit_vector.size() - 1));
}
