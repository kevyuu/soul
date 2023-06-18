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

auto generate_random_bool_vector(const soul_size size) -> std::vector<bool>
{
  std::random_device random_device;
  std::mt19937 random_engine(random_device());
  std::uniform_int_distribution<int> dist(0, 1);

  std::vector<bool> result;
  result.reserve(size);
  for (soul_size i = 0; i < size; i++) {
    result.push_back(static_cast<bool>(dist(random_engine)));
  }
  return result;
}

template <soul::ts_bit_block T>
auto get_vector_from_bit_vector(const soul::BitVector<T>& bit_vector) -> std::vector<bool>
{
  std::vector<bool> result;
  for (soul_size i = 0; i < bit_vector.size(); i++) {
    result.push_back(bit_vector[i]);
  }
  return result;
}

template <soul::ts_bit_block BlockType>
auto verify_sequence(
  const soul::BitVector<BlockType>& bit_vector, const std::vector<bool>& src_vector) -> void
{
  SOUL_TEST_ASSERT_EQ(bit_vector.size(), src_vector.size());
  SOUL_TEST_ASSERT_EQ(bit_vector.empty(), src_vector.empty());
  if (!src_vector.empty()) {
    SOUL_TEST_ASSERT_EQ(bit_vector.front(), src_vector.front());
    SOUL_TEST_ASSERT_EQ(bit_vector.back(), src_vector.back());
  }
  for (soul_size i = 0; i < src_vector.size(); i++) {
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
  SOUL_TEST_RUN(test_constructor<uint8>());
  SOUL_TEST_RUN(test_constructor<uint16>());
  SOUL_TEST_RUN(test_constructor<uint32>());
  SOUL_TEST_RUN(test_constructor<uint64>());
}

template <soul::ts_bit_block BlockType>
auto test_constructor_with_init_desc(const soul::BitVectorInitDesc init_desc) -> void
{
  const soul::BitVector<BlockType> bit_vector(init_desc);
  verify_sequence(bit_vector, std::vector<bool>(init_desc.size, init_desc.val));
  SOUL_TEST_ASSERT_GE(bit_vector.capacity(), bit_vector.size());
  SOUL_TEST_ASSERT_GE(bit_vector.capacity(), init_desc.capacity);
}

TEST(TestBitVectorConstruction, TestConstructorWithInitDesc)
{
  const soul::BitVector<> bit_vector(
    {.size = 10, .val = false, .capacity = 12}, *soul::get_default_allocator());
  SOUL_TEST_RUN(test_constructor_with_init_desc<uint8>({.size = 0}));
  SOUL_TEST_RUN(test_constructor_with_init_desc<uint8>({.size = 8}));
  SOUL_TEST_RUN(test_constructor_with_init_desc<uint8>({.size = 1, .val = true, .capacity = 10}));
  SOUL_TEST_RUN(test_constructor_with_init_desc<uint8>({.size = 8, .val = true, .capacity = 4}));

  SOUL_TEST_RUN(test_constructor_with_init_desc<uint64>({.size = 0}));
  SOUL_TEST_RUN(test_constructor_with_init_desc<uint64>({.size = 64}));
  SOUL_TEST_RUN(test_constructor_with_init_desc<uint64>({.size = 1, .capacity = 10}));
  SOUL_TEST_RUN(test_constructor_with_init_desc<uint64>({.size = 130, .capacity = 160}));
}

template <soul::ts_bit_block BlockType>
auto test_constructor_with_size(soul_size size) -> void
{
  const soul::BitVector<BlockType> bit_vector(size);
  SOUL_TEST_ASSERT_EQ(bit_vector.size(), size);
  SOUL_TEST_ASSERT_EQ(bit_vector.empty(), size == 0);
  for (soul_size i = 0; i < size; i++) {
    SOUL_TEST_ASSERT_EQ(bit_vector[i], false);
    SOUL_TEST_ASSERT_EQ(bit_vector.test(i, false), false);
    SOUL_TEST_ASSERT_EQ(bit_vector.test(i, true), false);
  }
  if (size != 0) {
    SOUL_TEST_ASSERT_FALSE(bit_vector.front());
    SOUL_TEST_ASSERT_FALSE(bit_vector.back());
  }
}

TEST(TestBitVectorConstruction, TestConstructorWithSize)
{
  SOUL_TEST_RUN(test_constructor_with_size<uint8>(0));
  SOUL_TEST_RUN(test_constructor_with_size<uint8>(8));
  SOUL_TEST_RUN(test_constructor_with_size<uint8>(1));
  SOUL_TEST_RUN(test_constructor_with_size<uint8>(20));

  SOUL_TEST_RUN(test_constructor_with_size<uint64>(0));
  SOUL_TEST_RUN(test_constructor_with_size<uint64>(64));
  SOUL_TEST_RUN(test_constructor_with_size<uint64>(1));
  SOUL_TEST_RUN(test_constructor_with_size<uint64>(130));
}

template <soul::ts_bit_block BlockType>
auto test_constructor_with_size_and_value(const soul_size size, const bool val) -> void
{
  const soul::BitVector<BlockType> bit_vector(size, val);
  SOUL_TEST_ASSERT_EQ(bit_vector.size(), size);
  SOUL_TEST_ASSERT_EQ(bit_vector.empty(), size == 0);
  for (soul_size i = 0; i < size; i++) {
    SOUL_TEST_ASSERT_EQ(bit_vector[i], val);
    SOUL_TEST_ASSERT_EQ(bit_vector.test(i, false), val);
    SOUL_TEST_ASSERT_EQ(bit_vector.test(i, true), val);
  }
  if (size != 0) {
    SOUL_TEST_ASSERT_EQ(bit_vector.front(), val);
    SOUL_TEST_ASSERT_EQ(bit_vector.back(), val);
  }
}

TEST(TestBitVectorConstruction, TestConstructorWithSizeAndValue)
{
  SOUL_TEST_RUN(test_constructor_with_size_and_value<uint8>(0, false));
  SOUL_TEST_RUN(test_constructor_with_size_and_value<uint8>(8, true));
  SOUL_TEST_RUN(test_constructor_with_size_and_value<uint8>(1, false));
  SOUL_TEST_RUN(test_constructor_with_size_and_value<uint8>(20, true));

  SOUL_TEST_RUN(test_constructor_with_size_and_value<uint64>(0, true));
  SOUL_TEST_RUN(test_constructor_with_size_and_value<uint64>(64, false));
  SOUL_TEST_RUN(test_constructor_with_size_and_value<uint64>(1, true));
  SOUL_TEST_RUN(test_constructor_with_size_and_value<uint64>(130, false));
}

template <soul::ts_bit_block BlockType>
auto test_constructor_with_bool_iterator(const soul_size size) -> void
{
  SOUL_TEST_ASSERT_NE(size, 0);
  const std::vector<bool> random_bool_vec = generate_random_bool_vector(size);

  soul::BitVector<BlockType> bit_vector(random_bool_vec.begin(), random_bool_vec.end());
  verify_sequence(bit_vector, random_bool_vec);
}

TEST(TestBitVectorConstruction, TestConstructorWithBoolIterator)
{
  SOUL_TEST_RUN(test_constructor_with_bool_iterator<uint8>(8));
  SOUL_TEST_RUN(test_constructor_with_bool_iterator<uint8>(1));
  SOUL_TEST_RUN(test_constructor_with_bool_iterator<uint8>(20));

  SOUL_TEST_RUN(test_constructor_with_bool_iterator<uint64>(64));
  SOUL_TEST_RUN(test_constructor_with_bool_iterator<uint64>(1));
  SOUL_TEST_RUN(test_constructor_with_bool_iterator<uint64>(130));
}

template <soul::ts_bit_block BlockType>
auto test_clone(const soul_size size) -> void
{
  const std::vector<bool> random_bool_vec = generate_random_bool_vector(size);
  const soul::BitVector<BlockType> src_bit_vector(random_bool_vec.begin(), random_bool_vec.end());
  const soul::BitVector<BlockType> test_bit_vector = src_bit_vector.clone();
  verify_sequence(test_bit_vector, random_bool_vec);
}

TEST(TestBitVectorConstruction, TestClone)
{
  SOUL_TEST_RUN(test_clone<uint8>(0));
  SOUL_TEST_RUN(test_clone<uint8>(8));
  SOUL_TEST_RUN(test_clone<uint8>(1));
  SOUL_TEST_RUN(test_clone<uint8>(20));

  SOUL_TEST_RUN(test_clone<uint64>(0));
  SOUL_TEST_RUN(test_clone<uint64>(64));
  SOUL_TEST_RUN(test_clone<uint64>(1));
  SOUL_TEST_RUN(test_clone<uint64>(130));
}

template <soul::ts_bit_block BlockType>
auto test_move_constructor(const soul_size size) -> void
{
  const std::vector<bool> random_bool_vec = generate_random_bool_vector(size);
  soul::BitVector<BlockType> src_bit_vector(random_bool_vec.begin(), random_bool_vec.end());
  const soul::BitVector<BlockType> test_bit_vector = std::move(src_bit_vector);
  verify_sequence(test_bit_vector, random_bool_vec);
}

TEST(TestBitVectorConstruction, TestMoveConstructor)
{
  SOUL_TEST_RUN(test_move_constructor<uint8>(0));
  SOUL_TEST_RUN(test_move_constructor<uint8>(8));
  SOUL_TEST_RUN(test_move_constructor<uint8>(1));
  SOUL_TEST_RUN(test_move_constructor<uint8>(20));

  SOUL_TEST_RUN(test_move_constructor<uint64>(0));
  SOUL_TEST_RUN(test_move_constructor<uint64>(64));
  SOUL_TEST_RUN(test_move_constructor<uint64>(1));
  SOUL_TEST_RUN(test_move_constructor<uint64>(130));
}

class TestBitVectorManipulation : public testing::Test
{
public:
  static constexpr soul_size RANDOM_BOOL_VECTOR_SIZE = 130;
  static constexpr soul_size TEST_CAPACITY = 250;

  TestBitVectorManipulation()
      : sources_vec(generate_random_bool_vector(RANDOM_BOOL_VECTOR_SIZE)),
        u8_filled_bit_vector(sources_vec.begin(), sources_vec.end()),
        u32_filled_bit_vector(sources_vec.begin(), sources_vec.end()),
        u64_filled_bit_vector(
          {.size = RANDOM_BOOL_VECTOR_SIZE, .val = true, .capacity = TEST_CAPACITY})
  {
  }

  std::vector<bool> sources_vec;
  soul::BitVector<> empty_bit_vector;

  soul::BitVector<uint8> u8_filled_bit_vector;
  soul::BitVector<uint32> u32_filled_bit_vector;
  soul::BitVector<uint64> u64_filled_bit_vector;
};

TEST_F(TestBitVectorManipulation, TestBitVectorResize)
{
  auto test_resize = []<soul::ts_bit_block BlockType>(
                       const soul::BitVector<BlockType>& bit_vector, const soul_size size) {
    auto test_vector = bit_vector.clone();
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
                        const soul::BitVector<BlockType>& sample_vector,
                        const soul_size new_capacity) {
    auto test_vector = sample_vector.clone();
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
    []<soul::ts_bit_block BlockType>(const soul::BitVector<BlockType>& sample_vector) {
      auto bit_vector = sample_vector.clone();
      bit_vector.clear();
      verify_sequence(bit_vector, std::vector<bool>());
    };

  SOUL_TEST_RUN(test_clear(empty_bit_vector));
  SOUL_TEST_RUN(test_clear(u8_filled_bit_vector));
  SOUL_TEST_RUN(test_clear(u32_filled_bit_vector));
  SOUL_TEST_RUN(test_clear(u64_filled_bit_vector));
}

TEST_F(TestBitVectorManipulation, TestBitVectorCleanup)
{
  auto test_cleanup =
    []<soul::ts_bit_block BlockType>(const soul::BitVector<BlockType>& sample_vector) {
      auto test_vector = sample_vector.clone();
      test_vector.cleanup();
      verify_sequence(test_vector, std::vector<bool>());
      SOUL_TEST_ASSERT_EQ(test_vector.capacity(), 0);
    };

  SOUL_TEST_RUN(test_cleanup(empty_bit_vector));
  SOUL_TEST_RUN(test_cleanup(u8_filled_bit_vector));
  SOUL_TEST_RUN(test_cleanup(u32_filled_bit_vector));
  SOUL_TEST_RUN(test_cleanup(u64_filled_bit_vector));
}

TEST_F(TestBitVectorManipulation, TestBitVectorPushBack)
{
  auto test_push_back = []<soul::ts_bit_block BlockType>(
                          const soul::BitVector<BlockType>& sample_vector, const bool val) {
    auto test_vector = sample_vector.clone();
    auto expected_vector = get_vector_from_bit_vector(test_vector);
    expected_vector.push_back(false);
    soul::BitRef bit_ref = test_vector.push_back();
    SOUL_TEST_ASSERT_EQ(bit_ref, false);
    verify_sequence(test_vector, expected_vector);
    expected_vector.back() = val;
    bit_ref = val;
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

  auto test_push_back_with_val = []<soul::ts_bit_block BlockType>(
                                   const soul::BitVector<BlockType>& sample_vector,
                                   const bool val) {
    auto test_vector = sample_vector.clone();
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
    []<soul::ts_bit_block BlockType>(const soul::BitVector<BlockType>& sample_vector) {
      auto test_vector = sample_vector.clone();
      auto expected_vector = get_vector_from_bit_vector(test_vector);
      expected_vector.pop_back();
      test_vector.pop_back();
      verify_sequence(test_vector, expected_vector);
    };

  SOUL_TEST_RUN(test_pop_back(u8_filled_bit_vector));
  SOUL_TEST_RUN(test_pop_back(u32_filled_bit_vector));
  SOUL_TEST_RUN(test_pop_back(u64_filled_bit_vector));

  auto test_pop_back_with_count = []<soul::ts_bit_block BlockType>(
                                    const soul::BitVector<BlockType>& sample_vector,
                                    const soul_size size) {
    auto test_vector = sample_vector.clone();
    auto expected_vector = get_vector_from_bit_vector(test_vector);
    for (soul_size i = 0; i < size; ++i) {
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
      const soul::BitVector<BlockType>& sample_vector, const soul_size index, const bool val) {
      auto test_vector = sample_vector.clone();
      auto expected_vector = get_vector_from_bit_vector(test_vector);
      if (expected_vector.size() <= index) {
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

  auto test_set =
    []<soul::ts_bit_block BlockType>(const soul::BitVector<BlockType>& sample_vector) {
      auto test_vector = sample_vector.clone();
      test_vector.set();
      verify_sequence(test_vector, std::vector<bool>(test_vector.size(), true));
    };

  SOUL_TEST_RUN(test_set(empty_bit_vector));
  SOUL_TEST_RUN(test_set(u8_filled_bit_vector));
  SOUL_TEST_RUN(test_set(u32_filled_bit_vector));
  SOUL_TEST_RUN(test_set(u64_filled_bit_vector));
}

TEST_F(TestBitVectorManipulation, TestBitVectorReset)
{
  auto test_reset =
    []<soul::ts_bit_block BlockType>(const soul::BitVector<BlockType>& sample_vector) {
      auto test_vector = sample_vector.clone();
      test_vector.reset();
      verify_sequence(test_vector, std::vector<bool>(test_vector.size(), false));
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
      const soul::BitVector<BlockType>& sample_vector, const soul_size idx, const bool val) {
      auto test_vector = sample_vector.clone();
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
      const soul::BitVector<BlockType>& sample_vector, const soul_size idx, const bool val) {
      auto test_vector = sample_vector.clone();
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
      const soul::BitVector<BlockType>& sample_vector, const soul_size idx, const bool val) {
      auto test_vector = sample_vector.clone();
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
                         const soul::BitVector<BlockType>& sample_vector, const soul_size idx) {
    auto test_vector = sample_vector.clone();
    auto expected_vector = get_vector_from_bit_vector(test_vector);
    expected_vector[idx] = !expected_vector[idx];
    const bool negate_val = ~test_vector[idx];
    test_vector[idx].flip();
    verify_sequence(test_vector, expected_vector);
    SOUL_TEST_ASSERT_EQ(negate_val, expected_vector[idx]);
  };

  SOUL_TEST_RUN(test_bit_flip(u8_filled_bit_vector, 5));
  SOUL_TEST_RUN(test_bit_flip(u8_filled_bit_vector, 0));
  SOUL_TEST_RUN(test_bit_flip(u8_filled_bit_vector, u8_filled_bit_vector.size() - 1));
}
