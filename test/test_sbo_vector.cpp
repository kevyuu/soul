#include <algorithm>
#include <list>
#include <memory>
#include <numeric>
#include <random>

#include <gtest/gtest.h>

#include "core/config.h"
#include "core/objops.h"
#include "core/sbo_vector.h"
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

using VectorInt = soul::SBOVector<int>;
using VectorObj = soul::SBOVector<TestObject, 4>;
using VectorListObj = soul::SBOVector<ListTestObject>;

constexpr usize CONSTRUCTOR_VECTOR_SIZE = 10;
constexpr int CONSTRUCTOR_VECTOR_DEFAULT_VALUE = 7;

template <typename T, usize N>
auto all_equal(const soul::SBOVector<T, N>& vec, const T& val) -> b8
{
  return std::ranges::all_of(vec, [&val](const T& x) { return x == val; });
}

template <typename T, usize N>
auto verify_sbo_vector(const soul::SBOVector<T, N>& vec1, const soul::SBOVector<T, N>& vec2)
{
  SOUL_TEST_ASSERT_EQ(vec1.size(), vec2.size());
  SOUL_TEST_ASSERT_EQ(vec1.empty(), vec2.empty());
  if (!vec1.empty()) {
    SOUL_TEST_ASSERT_EQ(vec1.front(), vec2.front());
    SOUL_TEST_ASSERT_EQ(vec1.back(), vec2.back());
  }
  SOUL_TEST_ASSERT_TRUE(std::equal(vec1.begin(), vec1.end(), vec2.begin()));
}

template <typename T, usize N>
auto generate_random_sbo_vector(const usize size) -> soul::SBOVector<T, N>
{
  const auto sequence = generate_random_sequence<T>(size);
  return soul::SBOVector<T, N>::from(sequence | soul::views::duplicate<T>());
}

template <typename T, usize N, std::ranges::input_range RangeT>
auto verify_sbo_vector(const soul::SBOVector<T, N>& vec, RangeT&& sequence)
{
  SOUL_TEST_ASSERT_EQ(vec.size(), sequence.size());
  SOUL_TEST_ASSERT_EQ(vec.empty(), sequence.empty());
  if (!sequence.empty()) {
    SOUL_TEST_ASSERT_EQ(vec.front(), *sequence.begin());
    SOUL_TEST_ASSERT_EQ(vec.back(), *(sequence.end() - 1));
  }
  SOUL_TEST_ASSERT_TRUE(std::equal(sequence.begin(), sequence.end(), vec.begin()));
}

template <typename T, usize VecSize, usize ArrSize>
auto verify_sbo_vector(const soul::SBOVector<T, VecSize>& vec, const std::array<T, ArrSize>& arr)
  -> b8
{
  return std::equal(vec.begin(), vec.end(), std::begin(arr));
}

template <typename T, usize N>
auto create_vector_from_sequence(const Sequence<T>& sequence) -> soul::SBOVector<T, N>
{
  return soul::SBOVector<T, N>::from(sequence | soul::views::duplicate<T>());
}

template <typename T, usize N>
auto create_vector_from_sequence(const Sequence<T>& sequence, soul::memory::Allocator& allocator)
  -> soul::SBOVector<T, N>
{
  return soul::SBOVector<T, N>::from(sequence | soul::views::duplicate<T>(), allocator);
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_constructor()
{
  soul::SBOVector<T, N> vector;
  SOUL_TEST_ASSERT_EQ(vector.size(), 0);
  SOUL_TEST_ASSERT_TRUE(vector.empty());
}

TEST(TestSBOVectorConstruction, TestDefaultConstructor)
{
  SOUL_TEST_RUN(test_constructor<int>());
  SOUL_TEST_RUN(test_constructor<TestObject>());
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_constructor<TestObject, 0>()));
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_constructor<TestObject, 4>()));
  SOUL_TEST_RUN(test_constructor<ListTestObject>());
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_constructor<ListTestObject, 0>()));
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_constructor<ListTestObject, 4>()));
}

TEST(TestSBOVectorConstruction, TestCustomAllocatorConstructor)
{
  TestObject::reset();
  TestAllocator::reset_all();
  TestAllocator test_allocator;

  static constexpr auto INLINE_ELEMENT_COUNT = 8;
  soul::SBOVector<int, INLINE_ELEMENT_COUNT, TestAllocator> vec_int(&test_allocator);
  ASSERT_TRUE(vec_int.empty());

  soul::SBOVector<TestObject, INLINE_ELEMENT_COUNT, TestAllocator> vec_to(&test_allocator);
  ASSERT_TRUE(vec_to.empty());

  soul::SBOVector<ListTestObject, INLINE_ELEMENT_COUNT, TestAllocator> vec_list_to(&test_allocator);
  ASSERT_TRUE(vec_list_to.empty());

  vec_int.resize(INLINE_ELEMENT_COUNT + 1);
  vec_to.resize(INLINE_ELEMENT_COUNT + 1);
  vec_list_to.resize(INLINE_ELEMENT_COUNT + 1);
  ASSERT_EQ(TestAllocator::allocCountAll, 3);
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_construction_with_size(const usize size)
{
  const auto vector = soul::SBOVector<T, N>::with_size(size);
  SOUL_TEST_ASSERT_EQ(vector.size(), size);
  SOUL_TEST_ASSERT_TRUE(all_equal(vector, T()));
}

TEST(TestSBOVectorConstruction, TestConstructorWithSize)
{
  SOUL_TEST_RUN(test_construction_with_size<int>(CONSTRUCTOR_VECTOR_SIZE));
  SOUL_TEST_RUN(test_construction_with_size<TestObject>(CONSTRUCTOR_VECTOR_SIZE));
  SOUL_TEST_RUN(
    SOUL_SINGLE_ARG(test_construction_with_size<TestObject, 4>(CONSTRUCTOR_VECTOR_SIZE)));
  SOUL_TEST_RUN(test_construction_with_size<ListTestObject>(CONSTRUCTOR_VECTOR_SIZE));
  SOUL_TEST_RUN(
    SOUL_SINGLE_ARG(test_construction_with_size<ListTestObject, 4>(CONSTRUCTOR_VECTOR_SIZE)));

  SOUL_TEST_RUN(test_construction_with_size<int>(0));
  SOUL_TEST_RUN(test_construction_with_size<TestObject>(0));
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_construction_with_size<TestObject, 4>(0)));
  SOUL_TEST_RUN(test_construction_with_size<ListTestObject>(0));
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_construction_with_size<ListTestObject, 4>(0)));
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_construction_with_capacity(const usize capacity)
{
  const auto vector = soul::Vector<T>::with_capacity(capacity);
  SOUL_TEST_ASSERT_EQ(vector.size(), 0);
  SOUL_TEST_ASSERT_GE(vector.capacity(), capacity);
}

TEST(TestSBOVectorConstruction, TestConstructionWithCapacity)
{
  auto type_set_test = []<typename T>() {
    constexpr auto default_inline_element_count =
      soul::get_sbo_vector_default_inline_element_count<T>();
    SOUL_TEST_RUN(test_construction_with_capacity<T>(default_inline_element_count - 1));
    SOUL_TEST_RUN(test_construction_with_capacity<T>(default_inline_element_count));
    SOUL_TEST_RUN(test_construction_with_capacity<T>(default_inline_element_count * 2));

    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_construction_with_capacity<T, 8>(8)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_construction_with_capacity<T, 8>(16)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_construction_with_capacity<T, 8>(2)));
  };

  SOUL_TEST_RUN(type_set_test.operator()<int>());
  SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
  SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_construction_fill_n(const usize size, const T& val)
{
  const auto vector = soul::SBOVector<T, N>::fill_n(size, val);
  SOUL_TEST_ASSERT_EQ(vector.size(), size);
  SOUL_TEST_ASSERT_TRUE(all_equal(vector, val));
  SOUL_TEST_ASSERT_EQ(vector.empty(), size == 0);
  if (size != 0) {
    SOUL_TEST_ASSERT_EQ(vector.front(), val);
    SOUL_TEST_ASSERT_EQ(vector.back(), val);
  }
}

TEST(TestSBOVectorConstruction, TestConstructionFillN)
{
  SOUL_TEST_RUN(
    test_construction_fill_n(CONSTRUCTOR_VECTOR_SIZE, CONSTRUCTOR_VECTOR_DEFAULT_VALUE));
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_construction_generate_n(const usize size, soul::ts_generate_fn<T> auto fn)
{
  T val = std::invoke(fn);
  const auto vector = soul::SBOVector<T, N>::generate_n(fn, size);
  SOUL_TEST_ASSERT_EQ(vector.size(), size);
  SOUL_TEST_ASSERT_TRUE(all_equal(vector, val));
}

TEST(TestSBOVectorConstruction, TestConstructionGenerateN)
{
  SOUL_TEST_RUN(test_construction_generate_n<int>(
    CONSTRUCTOR_VECTOR_SIZE, [] { return CONSTRUCTOR_VECTOR_DEFAULT_VALUE; }));

  static auto test_object_factory = [] { return TestObject(CONSTRUCTOR_VECTOR_DEFAULT_VALUE); };

  SOUL_TEST_RUN(
    test_construction_generate_n<TestObject>(CONSTRUCTOR_VECTOR_SIZE, test_object_factory));
  SOUL_TEST_RUN(test_construction_generate_n<ListTestObject>(CONSTRUCTOR_VECTOR_SIZE, [] {
    return ListTestObject::generate_n(test_object_factory, CONSTRUCTOR_VECTOR_SIZE);
  }));
}

TEST(TestVectorConstruction, TestVectorConstructionFromTransform)
{
  const auto vector = soul::SBOVector<TestObject>::transform(
    std::views::iota(0, 10), [](int val) { return TestObject(val); });

  SOUL_TEST_ASSERT_EQ(vector.size(), 10);
  for (auto i : std::views::iota(0, 10)) {
    SOUL_TEST_ASSERT_EQ(vector[i], TestObject(i));
  }
}

class TestSBOVectorConstructionWithSourceData : public testing::Test
{
public:
  VectorInt vector_int_src =
    VectorInt::fill_n(CONSTRUCTOR_VECTOR_SIZE, CONSTRUCTOR_VECTOR_DEFAULT_VALUE);
  VectorObj vector_to_src = VectorObj::generate_n(
    soul::clone_fn(TestObject(CONSTRUCTOR_VECTOR_DEFAULT_VALUE)), CONSTRUCTOR_VECTOR_SIZE);
  VectorListObj vector_list_to_src = VectorListObj::generate_n(
    soul::clone_fn(ListTestObject::generate_n(
      soul::clone_fn(TestObject(CONSTRUCTOR_VECTOR_DEFAULT_VALUE)), CONSTRUCTOR_VECTOR_SIZE)),
    CONSTRUCTOR_VECTOR_SIZE);
};

TEST_F(TestSBOVectorConstructionWithSourceData, TestClone)
{
  auto test_clone = []<typename T, usize N>(const soul::SBOVector<T, N>& vector_src) {
    soul::SBOVector<T, N> vector_dst =
      vector_src.clone(); // NOLINT(performance-unnecessary-copy-initialization)
    verify_sbo_vector(vector_dst, vector_src);
  };

  SOUL_TEST_RUN(test_clone(vector_int_src));
  SOUL_TEST_RUN(test_clone(vector_to_src));
  SOUL_TEST_RUN(test_clone(vector_list_to_src));
}

TEST_F(TestSBOVectorConstructionWithSourceData, TestCloneWithCustomAllocator)
{
  auto test_construction_with_custom_allocator =
    []<typename T, usize N>(const soul::SBOVector<T, N>& vector_src) {
      TestAllocator::reset_all();
      TestAllocator test_allocator;

      SOUL_TEST_ASSERT_EQ(test_allocator.allocCount, 0);
      soul::SBOVector<T, N> vector_dst = vector_src.clone(test_allocator);
      verify_sbo_vector(vector_dst, vector_src);
      SOUL_TEST_ASSERT_EQ(test_allocator.allocCount, 1);
    };

  SOUL_TEST_RUN(test_construction_with_custom_allocator(vector_int_src));
  SOUL_TEST_RUN(test_construction_with_custom_allocator(vector_to_src));
  SOUL_TEST_RUN(test_construction_with_custom_allocator(vector_list_to_src));
}

TEST_F(TestSBOVectorConstructionWithSourceData, TestMoveConstructor)
{
  auto test_move_constructor =
    []<typename T, usize N>(const soul::SBOVector<T, N>& vector_src) {
      soul::SBOVector<T, N> vector_src_copy = vector_src.clone();
      soul::SBOVector<T, N> vector_dst(std::move(vector_src_copy));
      verify_sbo_vector(vector_dst, vector_src);
    };

  SOUL_TEST_RUN(test_move_constructor(vector_int_src));
  SOUL_TEST_RUN(test_move_constructor(vector_to_src));
  SOUL_TEST_RUN(test_move_constructor(vector_list_to_src));
}

TEST_F(TestSBOVectorConstructionWithSourceData, TestConstructionFromRanges)
{
  auto test_construction_from_ranges =
    []<typename T, usize N>(const soul::SBOVector<T, N>& vector_src) {
      const auto vector_dst = soul::SBOVector<T, N>::from(vector_src | soul::views::duplicate<T>());
      verify_sbo_vector(vector_dst, vector_src);
    };

  SOUL_TEST_RUN(test_construction_from_ranges(vector_int_src));
  SOUL_TEST_RUN(test_construction_from_ranges(vector_to_src));
  SOUL_TEST_RUN(test_construction_from_ranges(vector_list_to_src));
}

TEST_F(TestSBOVectorConstructionWithSourceData, TestConstructionFromRangesWithCustomAllocator)
{
  auto test_iterator_constructor_with_custom_allocator =
    []<typename T, usize N>(const soul::SBOVector<T, N>& vector_src) {
      TestAllocator test_allocator;

      const auto vector_dst =
        soul::SBOVector<T, N>::from(vector_src | soul::views::duplicate<T>(), test_allocator);
      verify_sbo_vector(vector_dst, vector_src);
      SOUL_TEST_ASSERT_EQ(test_allocator.allocCount, 1);
    };

  SOUL_TEST_RUN(test_iterator_constructor_with_custom_allocator(vector_int_src));
  SOUL_TEST_RUN(test_iterator_constructor_with_custom_allocator(vector_to_src));
  SOUL_TEST_RUN(test_iterator_constructor_with_custom_allocator(vector_list_to_src));
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_vector_getter(const usize size)
{
  SOUL_TEST_ASSERT_NE(size, 0);
  const auto sequence = generate_random_sequence<T>(size);

  const auto middle = size / 2;

  const auto vector = create_vector_from_sequence<T, N>(sequence);
  SOUL_TEST_ASSERT_EQ(vector.front(), *sequence.begin());
  SOUL_TEST_ASSERT_EQ(vector.back(), *(sequence.end() - 1));
  SOUL_TEST_ASSERT_EQ(vector[middle], *(sequence.begin() + middle));
}

TEST(TestSBOVectorGetter, TestSBOVectorGetter)
{
  SOUL_TEST_RUN(test_vector_getter<int>(7));
  SOUL_TEST_RUN(test_vector_getter<TestObject>(9));
  SOUL_TEST_RUN(test_vector_getter<ListTestObject>(7));
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_set_allocator(const usize vec_size)
{
  TestAllocator test_allocator;
  const auto sequence = generate_random_sequence<T>(vec_size);
  auto vector = create_vector_from_sequence<T, N>(sequence);
  vector.set_allocator(test_allocator);
  SOUL_TEST_ASSERT_EQ(vector.get_allocator(), &test_allocator);
  verify_sbo_vector(vector, sequence);
}

TEST(TestSBOVectorSetAllocator, TestSBOVectorSetAllocator)
{
  auto type_set_test = []<typename T>() {
    SOUL_TEST_RUN(test_set_allocator<T>(0));
    SOUL_TEST_RUN(test_set_allocator<T>(20));
    SOUL_TEST_RUN(test_set_allocator<T>(1));

    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_set_allocator<T, 8>(0)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_set_allocator<T, 8>(1)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_set_allocator<T, 8>(8)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_set_allocator<T, 8>(10)));
  };

  SOUL_TEST_RUN(type_set_test.operator()<int>());
  SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
  SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_copy_assignment_operator(const usize src_size, const usize dst_size)
{
  TestAllocator test_allocator("Test Allocator For Copy Assignment Operator");
  const auto src_sequence = generate_random_sequence<T>(src_size);
  const auto dst_sequence = generate_random_sequence<T>(dst_size);

  const auto src_vector = create_vector_from_sequence<T, N>(src_sequence);
  auto dst_vector = create_vector_from_sequence<T, N>(dst_sequence, test_allocator);

  dst_vector = src_vector.clone();

  verify_sbo_vector(dst_vector, src_sequence);
  verify_sbo_vector(src_vector, src_sequence);
  SOUL_TEST_ASSERT_EQ(dst_vector.get_allocator(), &test_allocator);
};

TEST(TestSBOVectorCopyAssignmentOperator, TestSBOVectorCopyAssignmentOperator)
{
  auto type_set_test = []<typename T>() {
    auto src_size_set_test = [](const usize src_size) {
      constexpr auto default_inline_count = soul::get_sbo_vector_default_inline_element_count<T>();
      SOUL_TEST_RUN(test_copy_assignment_operator<T>(
        src_size, std::max<usize>(default_inline_count / 2, 1)));
      SOUL_TEST_RUN(test_copy_assignment_operator<T>(src_size, default_inline_count * 2));
      SOUL_TEST_RUN(test_copy_assignment_operator<T>(src_size, default_inline_count));

      constexpr auto custom_inline_count = 8;
      SOUL_TEST_RUN(SOUL_SINGLE_ARG(
        test_copy_assignment_operator<T, custom_inline_count>(src_size, custom_inline_count / 2)));
      SOUL_TEST_RUN(SOUL_SINGLE_ARG(
        test_copy_assignment_operator<T, custom_inline_count>(src_size, custom_inline_count * 2)));
      SOUL_TEST_RUN(SOUL_SINGLE_ARG(
        test_copy_assignment_operator<T, custom_inline_count>(src_size, custom_inline_count)));
    };

    SOUL_TEST_RUN(src_size_set_test(0));
    SOUL_TEST_RUN(src_size_set_test(6));
    SOUL_TEST_RUN(src_size_set_test(8));
    SOUL_TEST_RUN(src_size_set_test(100));
  };

  SOUL_TEST_RUN(type_set_test.operator()<int>());
  SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
  SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_move_assignment_operator(const usize src_size, const usize dst_size)
{
  TestAllocator test_allocator("Test Allocator For Move Assignment Operator");
  const auto src_sequence = generate_random_sequence<T>(src_size);
  const auto dst_sequence = generate_random_sequence<T>(dst_size);

  auto src_vector = create_vector_from_sequence<T, N>(src_sequence);
  auto dst_vector = create_vector_from_sequence<T, N>(dst_sequence, test_allocator);

  dst_vector = std::move(src_vector);

  verify_sbo_vector(dst_vector, src_sequence);
  SOUL_TEST_ASSERT_EQ(dst_vector.get_allocator(), &test_allocator);
};

TEST(TestSBOVectorMoveAssingmentOperator, TestSBOVectorMoveAssignmentOperator)
{
  auto type_set_test = []<typename T>() {
    auto src_size_set_test = [](const usize src_size) {
      constexpr auto default_inline_count = soul::get_sbo_vector_default_inline_element_count<T>();
      SOUL_TEST_RUN(test_move_assignment_operator<T>(
        src_size, std::max<usize>(default_inline_count / 2, 1)));
      SOUL_TEST_RUN(test_move_assignment_operator<T>(src_size, default_inline_count * 2));
      SOUL_TEST_RUN(test_move_assignment_operator<T>(src_size, default_inline_count));

      constexpr auto custom_inline_count = 8;
      SOUL_TEST_RUN(SOUL_SINGLE_ARG(
        test_move_assignment_operator<T, custom_inline_count>(src_size, custom_inline_count / 2)));
      SOUL_TEST_RUN(SOUL_SINGLE_ARG(
        test_move_assignment_operator<T, custom_inline_count>(src_size, custom_inline_count * 2)));
      SOUL_TEST_RUN(SOUL_SINGLE_ARG(
        test_move_assignment_operator<T, custom_inline_count>(src_size, custom_inline_count)));
    };

    SOUL_TEST_RUN(src_size_set_test(0));
    SOUL_TEST_RUN(src_size_set_test(6));
    SOUL_TEST_RUN(src_size_set_test(8));
    SOUL_TEST_RUN(src_size_set_test(100));
  };

  SOUL_TEST_RUN(type_set_test.operator()<int>());
  SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
  SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_assign_with_size_and_value(
  const usize vec_size, const usize assign_size, const T& assign_val)
{
  auto vector = generate_random_sbo_vector<T, N>(vec_size);
  vector.assign(assign_size, assign_val);
  verify_sbo_vector(vector, generate_sequence(assign_size, assign_val));
}

TEST(TestSBOVectorAssignWithSizeAndValue, TestSBOVectorAssignWithSizeAndValue)
{
  static constexpr auto ASSIGN_VECTOR_DEFAULT_VALUE = 8;

  auto type_set_test = []<typename T>() {
    constexpr auto default_inline_element_count =
      soul::get_sbo_vector_default_inline_element_count<T>();
    const T default_val(ASSIGN_VECTOR_DEFAULT_VALUE);

    SOUL_TEST_RUN(test_assign_with_size_and_value<T>(1, default_inline_element_count, default_val));
    SOUL_TEST_RUN(
      test_assign_with_size_and_value<T>(1, default_inline_element_count * 2, default_val));

    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_size_and_value<T, 8>(0, 8, default_val)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_size_and_value<T, 8>(0, 16, default_val)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_size_and_value<T, 8>(1, 8, default_val)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_size_and_value<T, 8>(8, 6, default_val)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_size_and_value<T, 8>(8, 16, default_val)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_size_and_value<T, 8>(16, 8, default_val)));
  };

  SOUL_TEST_RUN(type_set_test.operator()<int>());
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_assign_with_ranges(const usize vec_size, const usize assign_size)
{
  auto vector = generate_random_sbo_vector<T, N>(vec_size);
  const auto sequence = generate_random_sequence<T>(assign_size);
  vector.assign(sequence | soul::views::duplicate<T>());
  verify_sbo_vector(vector, sequence);
}

TEST(TestSBOVectorAssignWithIterator, TestSBOVectorAssignWithIterator)
{
  auto type_set_test = []<typename T>() {
    constexpr auto default_inline_element_count =
      soul::get_sbo_vector_default_inline_element_count<T>();
    SOUL_TEST_RUN(test_assign_with_ranges<T>(0, default_inline_element_count));
    SOUL_TEST_RUN(test_assign_with_ranges<T>(0, default_inline_element_count * 2));

    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_ranges<T, 8>(4, 2)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_ranges<T, 8>(4, 8)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_ranges<T, 8>(4, 16)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_ranges<T, 8>(10, 2)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_ranges<T, 8>(10, 9)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_ranges<T, 8>(10, 16)));
  };

  SOUL_TEST_RUN(type_set_test.operator()<int>());
  SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
  SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_swap(const usize size1, const usize size2)
{
  const auto sequence1 = generate_random_sequence<T>(size1);
  const auto sequence2 = generate_random_sequence<T>(size2);

  auto vector1 = create_vector_from_sequence<T, N>(sequence1);
  auto vector2 = create_vector_from_sequence<T, N>(sequence2);
  vector1.swap(vector2);
  verify_sbo_vector(vector1, sequence2);
  verify_sbo_vector(vector2, sequence1);

  swap(vector1, vector2);
  verify_sbo_vector(vector1, sequence1);
  verify_sbo_vector(vector2, sequence2);
}

TEST(TestSBOVectorSwap, TestSBOVectorSwap)
{
  auto type_set_test = []<typename T>() {
    constexpr auto default_inline_element_count =
      soul::get_sbo_vector_default_inline_element_count<T>();

    SOUL_TEST_RUN(test_swap<T>(0, default_inline_element_count));
    SOUL_TEST_RUN(test_swap<T>(0, default_inline_element_count * 2));

    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_swap<T, 8>(2, 3)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_swap<T, 8>(2, 16)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_swap<T, 8>(16, 32)));
  };

  SOUL_TEST_RUN(type_set_test.operator()<int>());
  SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
  SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_resize(const usize vec_size, const usize resize_size)
{
  const auto original_sequence = generate_random_sequence<T>(vec_size);
  auto vector = create_vector_from_sequence<T, N>(original_sequence);

  const auto resize_sequence = [vec_size, resize_size, &original_sequence]() {
    if (resize_size > vec_size) {
      return generate_sequence(original_sequence, generate_sequence(resize_size - vec_size, T()));
    }
    return generate_sequence<T>(original_sequence.begin(), original_sequence.begin() + resize_size);
  }();

  if constexpr (std::same_as<T, TestObject> || std::same_as<T, ListTestObject>) {
    TestObject::reset();
  }
  vector.resize(resize_size);
  verify_sbo_vector(vector, resize_sequence);
  if (vec_size > resize_size) {
    if constexpr (std::same_as<T, TestObject>) {
      SOUL_TEST_ASSERT_EQ(
        TestObject::sTODtorCount - TestObject::sTOCtorCount, vec_size - resize_size);
    }
    if constexpr (std::same_as<T, ListTestObject>) {
      const auto destructed_objects_count = std::accumulate(
        original_sequence.begin() + resize_size,
        original_sequence.end(),
        usize(0),
        [](const usize prev, const ListTestObject& curr) { return prev + curr.size(); });
      SOUL_TEST_ASSERT_EQ(
        TestObject::sTODtorCount - TestObject::sTOCtorCount, destructed_objects_count);
    }
  }
}

TEST(TestSBOVectorResize, TestSBOVectorResize)
{
  auto type_set_test = []<typename T>() {
    constexpr auto default_inline_element_count =
      soul::get_sbo_vector_default_inline_element_count<T>();

    SOUL_TEST_RUN(test_resize<T>(0, default_inline_element_count));
    SOUL_TEST_RUN(test_resize<T>(0, default_inline_element_count * 2));
    SOUL_TEST_RUN(test_resize<T>(default_inline_element_count, 0));
    SOUL_TEST_RUN(test_resize<T>(default_inline_element_count, 1));
    SOUL_TEST_RUN(test_resize<T>(default_inline_element_count * 2, 0));
    SOUL_TEST_RUN(test_resize<T>(default_inline_element_count * 2, default_inline_element_count));

    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(0, 8)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(0, 16)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(8, 0)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(8, 4)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(8, 16)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(6, 8)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(10, 2)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(10, 9)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(10, 16)));
  };

  SOUL_TEST_RUN(type_set_test.operator()<int>());
  SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
  SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_reserve(const usize vec_size, const usize new_capacity)
{
  const auto sequence = generate_random_sequence<T>(vec_size);
  auto vector = create_vector_from_sequence<T, N>(sequence);
  const auto old_capacity = vector.capacity();
  vector.reserve(new_capacity);
  verify_sbo_vector(vector, sequence);
  if (old_capacity >= new_capacity) {
    SOUL_TEST_ASSERT_EQ(vector.capacity(), old_capacity);
  } else {
    SOUL_TEST_ASSERT_EQ(vector.capacity(), new_capacity);
  }
}

TEST(TestSBOVectorReserve, TestSBOVectorReserve)
{
  auto type_set_test = []<typename T>() {
    constexpr auto default_inline_element_count =
      soul::get_sbo_vector_default_inline_element_count<T>();
    SOUL_TEST_RUN(test_reserve<T>(0, default_inline_element_count - 1));
    SOUL_TEST_RUN(test_reserve<T>(0, default_inline_element_count));
    SOUL_TEST_RUN(test_reserve<T>(0, default_inline_element_count * 2));

    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_reserve<T, 8>(4, 8)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_reserve<T, 8>(4, 16)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_reserve<T, 8>(4, 2)));

    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_reserve<T, 8>(16, 8)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_reserve<T, 8>(16, 32)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_reserve<T, 8>(16, 2)));
  };

  SOUL_TEST_RUN(type_set_test.operator()<int>());
  SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
  SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_push_back(const usize vec_size, const T& val)
{
  using Vector = soul::SBOVector<T, N>;
  const auto sequence = generate_random_sequence<T>(vec_size);
  auto vector = create_vector_from_sequence<T, N>(sequence);

  const auto push_back_sequence = generate_sequence(sequence, generate_sequence(1, val));

  Vector test_copy1 = vector.clone();
  Vector test_copy2 = vector.clone();

  if constexpr (!soul::ts_clone<T>) {
    test_copy1.push_back(val);
    verify_sbo_vector(test_copy1, push_back_sequence);
  }

  T val_copy = soul::duplicate(val);
  test_copy2.push_back(std::move(val_copy));
  verify_sbo_vector(test_copy2, push_back_sequence);
}

TEST(TestSBOVectorPushback, TestSBOVectorPushBack)
{
  auto type_set_test = []<typename T>() {
    const T val = []() {
      if constexpr (std::same_as<T, ListTestObject>) {
        return ListTestObject::generate_n(soul::clone_fn(TestObject(5)), 10);
      } else {
        return T(5);
      }
    }();
    SOUL_TEST_RUN(test_push_back<T>(0, val));

    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_push_back<T, 8>(0, val)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_push_back<T, 8>(7, val)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_push_back<T, 8>(8, val)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_push_back<T, 8>(12, val)));
  };

  SOUL_TEST_RUN(type_set_test.operator()<int>());
  SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
  SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_generate_back(const usize vec_size, soul::ts_generate_fn<T> auto fn)
{
  T val = std::invoke(fn);
  using Vector = soul::SBOVector<T, N>;
  const auto sequence = generate_random_sequence<T>(vec_size);
  auto test_vector = create_vector_from_sequence<T, N>(sequence);

  const auto generate_back_sequence = generate_sequence(sequence, generate_sequence(1, val));

  test_vector.generate_back(std::move(fn));
  verify_sbo_vector(test_vector, generate_back_sequence);
};

TEST(TestSBOVectorGenerateBack, TestSBOVectorGenerateBack)
{

  auto type_set_test = []<typename T>() {
    auto val_fn = []() -> T {
      if constexpr (std::same_as<T, ListTestObject>) {
        return ListTestObject::generate_n(soul::clone_fn(TestObject(5)), 10);
      } else {
        return T(5);
      }
    };
    SOUL_TEST_RUN(test_generate_back<T>(0, val_fn));

    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_generate_back<T, 8>(0, val_fn)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_generate_back<T, 8>(7, val_fn)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_generate_back<T, 8>(8, val_fn)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_generate_back<T, 8>(12, val_fn)));
  };

  SOUL_TEST_RUN(type_set_test.operator()<int>());
  SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
  SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_pop_back(const usize vec_size)
{
  const auto sequence = generate_random_sequence<T>(vec_size);
  auto vector = create_vector_from_sequence<T, N>(sequence);
  if constexpr (std::same_as<T, TestObject>) {
    TestObject::reset();
  }
  vector.pop_back();
  verify_sbo_vector(vector, generate_sequence<T>(sequence.begin(), sequence.end() - 1));
  if constexpr (std::same_as<T, TestObject>) {
    SOUL_TEST_ASSERT_EQ(TestObject::sTODtorCount - TestObject::sTOCtorCount, 1);
  }
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_pop_back_with_size(const usize vec_size, const usize pop_back_size)
{
  const auto sequence = generate_random_sequence<T>(vec_size);
  auto vector = create_vector_from_sequence<T, N>(sequence);
  if constexpr (std::same_as<T, TestObject>) {
    TestObject::reset();
  }
  vector.pop_back(pop_back_size);
  verify_sbo_vector(vector, generate_sequence<T>(sequence.begin(), sequence.end() - pop_back_size));
  if constexpr (std::same_as<T, TestObject>) {
    SOUL_TEST_ASSERT_EQ(TestObject::sTODtorCount - TestObject::sTOCtorCount, pop_back_size);
  }
}

TEST(TestSBOVectorPopBack, TestSBOVectorPopBack)
{
  auto type_set_test = []<typename T>() {
    constexpr auto default_inline_element_count =
      soul::get_sbo_vector_default_inline_element_count<T>();
    SOUL_TEST_RUN(test_pop_back<T>(1));
    SOUL_TEST_RUN(test_pop_back<T>(default_inline_element_count));
    SOUL_TEST_RUN(test_pop_back<T>(default_inline_element_count + 1));

    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_pop_back<T, 8>(8)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_pop_back<T, 8>(9)));

    SOUL_TEST_RUN(test_pop_back_with_size<T>(2, 2));
    SOUL_TEST_RUN(test_pop_back_with_size<T>(
      default_inline_element_count + 2, default_inline_element_count + 2));
    SOUL_TEST_RUN(test_pop_back_with_size<T>(
      default_inline_element_count + 2, default_inline_element_count + 1));

    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_pop_back_with_size<T, 8>(4, 4)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_pop_back_with_size<T, 8>(4, 2)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_pop_back_with_size<T, 8>(16, 8)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_pop_back_with_size<T, 8>(16, 4)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_pop_back_with_size<T, 8>(16, 16)));
  };

  SOUL_TEST_RUN(type_set_test.operator()<int>());
  SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
  SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

TEST(TestSBOVectorEmplaceBack, TestSBOVectorEmplaceBack)
{
  auto test_emplace_back = []<usize N>(const usize vec_size) {
    const auto vector_sequence = generate_random_sequence<TestObject>(vec_size);
    auto vector = create_vector_from_sequence<TestObject, N>(vector_sequence);

    soul::SBOVector<TestObject, N> test_copy1 = vector.clone();
    test_copy1.emplace_back(3);
    SOUL_TEST_ASSERT_EQ(test_copy1.size(), vector.size() + 1);
    SOUL_TEST_ASSERT_EQ(test_copy1.back(), TestObject(3));
    SOUL_TEST_ASSERT_TRUE(std::equal(vector.cbegin(), vector.cend(), test_copy1.cbegin()));

    soul::SBOVector<TestObject, N> test_copy2 = vector.clone();
    test_copy2.emplace_back(4, 5, 6);
    SOUL_TEST_ASSERT_EQ(test_copy2.size(), vector.size() + 1);
    SOUL_TEST_ASSERT_EQ(test_copy2.back(), TestObject(4 + 5 + 6));
    SOUL_TEST_ASSERT_TRUE(std::equal(vector.cbegin(), vector.cend(), test_copy2.cbegin()));
  };

  SOUL_TEST_RUN(test_emplace_back.operator()<1>(0));
  SOUL_TEST_RUN(test_emplace_back.operator()<8>(7));
  SOUL_TEST_RUN(test_emplace_back.operator()<8>(8));
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_append(const usize vec_size, const usize append_size)
{
  const auto vector_sequence = generate_random_sequence<T>(vec_size);
  auto vector = create_vector_from_sequence<T, N>(vector_sequence);

  const auto append_sequence = generate_random_sequence<T>(append_size);

  using Vector = soul::SBOVector<T, N>;

  Vector test_copy1 = vector.clone();
  test_copy1.append(append_sequence | soul::views::duplicate<T>());
  SOUL_TEST_ASSERT_EQ(test_copy1.size(), vector.size() + append_size);
  SOUL_TEST_ASSERT_TRUE(std::equal(vector.begin(), vector.end(), test_copy1.begin()));
  SOUL_TEST_ASSERT_TRUE(std::equal(
    test_copy1.begin() + vector.size(),
    test_copy1.end(),
    append_sequence.begin(),
    append_sequence.end()));

  auto append_src_vec = create_vector_from_sequence<T, N>(append_sequence);
  Vector test_copy2 = vector.clone();
  test_copy2.append(append_src_vec | soul::views::duplicate<T>());
  SOUL_TEST_ASSERT_EQ(test_copy2.size(), test_copy1.size());
  SOUL_TEST_ASSERT_TRUE(std::ranges::equal(test_copy1, test_copy2));
}

TEST(TestSBOVectorAppend, TestSBOVectorAppend)
{
  auto type_set_test = []<typename T>() {
    SOUL_TEST_RUN(test_append<T>(0, 1));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_append<T, 8>(3, 2)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_append<T, 8>(3, 4)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_append<T, 8>(3, 5)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_append<T, 8>(3, 16)));
  };

  SOUL_TEST_RUN(type_set_test.operator()<int>());
  SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
  SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_clear(const usize size)
{
  const auto sequence = generate_random_sequence<T>(size);
  auto vector = create_vector_from_sequence<T, N>(sequence);

  const usize old_capacity = vector.capacity();
  const usize old_size = vector.size();
  if constexpr (std::same_as<T, TestObject>) {
    TestObject::reset();
  }
  vector.clear();
  SOUL_TEST_ASSERT_EQ(vector.size(), 0);
  SOUL_TEST_ASSERT_EQ(vector.capacity(), old_capacity);
  if constexpr (std::same_as<T, TestObject>) {
    SOUL_TEST_ASSERT_EQ(TestObject::sTODtorCount - TestObject::sTOCtorCount, old_size);
  }
}

TEST(TestSBOVectorClear, TestSBOVectorClear)
{
  auto type_set_test = []<typename T>() {
    SOUL_TEST_RUN(test_clear<T>(0));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_clear<T, 8>(4)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_clear<T, 8>(8)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_clear<T, 8>(16)));
  };

  SOUL_TEST_RUN(type_set_test.operator()<int>());
  SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
  SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, usize N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_cleanup(const usize size)
{
  const auto sequence = generate_random_sequence<T>(size);
  auto vector = create_vector_from_sequence<T, N>(sequence);

  const usize old_size = vector.size();
  using VecType = soul::SBOVector<T, N>;
  if constexpr (std::same_as<T, TestObject>) {
    TestObject::reset();
  }
  vector.cleanup();
  SOUL_TEST_ASSERT_EQ(vector.size(), 0);
  SOUL_TEST_ASSERT_EQ(vector.capacity(), VecType::INLINE_ELEMENT_COUNT);
  if constexpr (std::same_as<T, TestObject>) {
    SOUL_TEST_ASSERT_EQ(TestObject::sTODtorCount - TestObject::sTOCtorCount, old_size);
  }
}

TEST(TestSBOVectorCleanup, TestSBOVectorCleanup)
{
  auto type_set_test = []<typename T>() {
    SOUL_TEST_RUN(test_cleanup<T>(0));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_cleanup<T, 8>(4)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_cleanup<T, 8>(8)));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_cleanup<T, 8>(16)));
  };

  SOUL_TEST_RUN(type_set_test.operator()<int>());
  SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
  SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}
