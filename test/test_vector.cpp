#include <algorithm>
#include <array>
#include <list>
#include <numeric>
#include <random>

#include <gtest/gtest.h>

#include "core/config.h"
#include "core/vector.h"
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

using VectorInt = soul::Vector<int>;
using VectorObj = soul::Vector<TestObject>;
using VectorListObj = soul::Vector<ListTestObject>;

constexpr usize CONSTRUCTOR_VECTOR_SIZE = 10;
constexpr int CONSTRUCTOR_VECTOR_DEFAULT_VALUE = 7;

template <typename T>
static auto generate_random_array(T* arr, usize size) -> void
{
  std::random_device random_device;
  std::mt19937 random_engine(random_device());
  std::uniform_int_distribution<int> dist(1, 100);
  for (usize i = 0; i < size; i++) {
    if constexpr (std::same_as<T, ListTestObject>) {
      arr[i] = T::with_size(dist(random_engine));
    } else {
      arr[i] = T(dist(random_engine));
    }
  }
}

template <typename T>
auto all_equal(const soul::Vector<T>& vec, const T& val) -> b8
{
  return std::ranges::all_of(vec, [&val](const T& x) { return x == val; });
}

template <typename T, usize N>
auto verify_vector(const soul::Vector<T>& vec, const std::array<T, N>& arr) -> b8
{
  return std::equal(vec.begin(), vec.end(), std::begin(arr));
}

template <typename T>
auto test_default_constructor() -> void
{
  soul::Vector<T> vector;
  SOUL_TEST_ASSERT_TRUE(vector.empty());
}

TEST(TestVectorConstruction, TestDefaultConstructor)
{
  SOUL_TEST_RUN(test_default_constructor<int>());
  SOUL_TEST_RUN(test_default_constructor<TestObject>());
  SOUL_TEST_RUN(test_default_constructor<ListTestObject>());
}

TEST(TestVectorConstruction, TestCustomAllocatorConstructor)
{
  TestObject::reset();
  TestAllocator::reset_all();
  TestAllocator test_allocator;

  soul::Vector<int, TestAllocator> vec_int(&test_allocator);
  ASSERT_TRUE(vec_int.empty());

  soul::Vector<TestObject, TestAllocator> vec_to(&test_allocator);
  ASSERT_TRUE(vec_to.empty());

  soul::Vector<ListTestObject, TestAllocator> vec_list_to(&test_allocator);
  ASSERT_TRUE(vec_list_to.empty());

  vec_int.resize(1);
  vec_to.resize(1);
  vec_list_to.resize(1);
  ASSERT_EQ(TestAllocator::allocCountAll, 3);
}

template <typename T>
auto test_construction_with_size(const usize size) -> void
{
  auto vector = soul::Vector<T>::with_size(size);
  SOUL_TEST_ASSERT_EQ(vector.size(), size);
  SOUL_TEST_ASSERT_TRUE(all_equal(vector, T()));
}

TEST(TestVectorConstruction, TestConstructionWithSize)
{
  SOUL_TEST_RUN(test_construction_with_size<int>(CONSTRUCTOR_VECTOR_SIZE));
  SOUL_TEST_RUN(test_construction_with_size<TestObject>(CONSTRUCTOR_VECTOR_SIZE));
  SOUL_TEST_RUN(test_construction_with_size<ListTestObject>(CONSTRUCTOR_VECTOR_SIZE));

  SOUL_TEST_RUN(test_construction_with_size<int>(0));
  SOUL_TEST_RUN(test_construction_with_size<TestObject>(0));
  SOUL_TEST_RUN(test_construction_with_size<ListTestObject>(0));
}

template <typename T>
auto test_construction_fill_n(const usize size, const T& val) -> void
{
  auto vector = soul::Vector<T>::fill_n(size, val);
  SOUL_TEST_ASSERT_EQ(vector.size(), size);
  SOUL_TEST_ASSERT_TRUE(all_equal(vector, val));
}

TEST(TestVectorConstruction, TestConstructorFillN)
{
  SOUL_TEST_RUN(
    test_construction_fill_n(CONSTRUCTOR_VECTOR_SIZE, CONSTRUCTOR_VECTOR_DEFAULT_VALUE));
}

template <typename T, soul::ts_generate_fn<T> Fn>
void test_construction_generate_n(Fn fn, const usize size)
{
  T val = std::invoke(fn);
  const auto vector = soul::Vector<T>::generate_n(fn, size);
  SOUL_TEST_ASSERT_EQ(vector.size(), size);
  SOUL_TEST_ASSERT_TRUE(all_equal(vector, val));
}

TEST(TestVectorConstruction, TestConstructionGenerateN)
{
  SOUL_TEST_RUN(test_construction_generate_n<int>(
    [] { return CONSTRUCTOR_VECTOR_DEFAULT_VALUE; }, CONSTRUCTOR_VECTOR_SIZE));

  static auto test_object_factory = [] { return TestObject(CONSTRUCTOR_VECTOR_DEFAULT_VALUE); };

  SOUL_TEST_RUN(
    test_construction_generate_n<TestObject>(test_object_factory, CONSTRUCTOR_VECTOR_SIZE));
  SOUL_TEST_RUN(test_construction_generate_n<ListTestObject>(
    [] { return ListTestObject::generate_n(test_object_factory, CONSTRUCTOR_VECTOR_SIZE); },
    CONSTRUCTOR_VECTOR_SIZE));
}

template <typename T>
void test_construction_with_capacity(const usize capacity)
{
  const auto vector = soul::Vector<T>::with_capacity(capacity);
  SOUL_TEST_ASSERT_EQ(vector.size(), 0);
  SOUL_TEST_ASSERT_EQ(vector.capacity(), capacity);
}

TEST(TestVectorConstruction, TestConstructionWithCapacity)
{
  SOUL_TEST_RUN(test_construction_with_capacity<int>(5));

  SOUL_TEST_RUN(test_construction_with_capacity<TestObject>(0));
  SOUL_TEST_RUN(test_construction_with_capacity<TestObject>(10));

  SOUL_TEST_RUN(test_construction_with_capacity<ListTestObject>(0));
  SOUL_TEST_RUN(test_construction_with_capacity<ListTestObject>(20));
}

TEST(TestVectorConstruction, TestVectorConstructionFromTransform)
{
  const auto vector = soul::Vector<TestObject>::transform(
    std::views::iota(0, 10), [](int val) { return TestObject(val); });

  SOUL_TEST_ASSERT_EQ(vector.size(), 10);
  for (auto i : std::views::iota(0, 10)) {
    SOUL_TEST_ASSERT_EQ(vector[i], TestObject(i));
  }
}

class TestVectorConstructionWithSourceData : public testing::Test
{
public:
  VectorInt vectorIntSrc =
    VectorInt::fill_n(CONSTRUCTOR_VECTOR_SIZE, CONSTRUCTOR_VECTOR_DEFAULT_VALUE);
  VectorObj vectorToSrc = VectorObj::generate_n(
    [] { return TestObject(CONSTRUCTOR_VECTOR_DEFAULT_VALUE); }, CONSTRUCTOR_VECTOR_SIZE);
  soul::Vector<ListTestObject> vectorListToSrc = soul::Vector<ListTestObject>::generate_n(
    [] { return ListTestObject::with_size(CONSTRUCTOR_VECTOR_SIZE); }, CONSTRUCTOR_VECTOR_SIZE);
};

TEST_F(TestVectorConstructionWithSourceData, TestClone)
{
  auto test_clone = []<typename T>(const soul::Vector<T>& vector_src) {
    soul::Vector<T> vector_dst = vector_src.clone();
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(vector_dst, vector_src));
  };

  SOUL_TEST_RUN(test_clone(vectorIntSrc));
  SOUL_TEST_RUN(test_clone(vectorToSrc));
  SOUL_TEST_RUN(test_clone(vectorListToSrc));
}

TEST_F(TestVectorConstructionWithSourceData, TestCloneWithCustomAllocator)
{
  auto test_clone_with_custom_allocator = []<typename T>(const soul::Vector<T>& vector_src) {
    TestAllocator::reset_all();
    TestAllocator test_allocator;

    SOUL_TEST_ASSERT_EQ(test_allocator.allocCount, 0);
    soul::Vector<T> vector_dst = vector_src.clone(test_allocator);
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(vector_src, vector_dst));
    SOUL_TEST_ASSERT_EQ(test_allocator.allocCount, 1);
  };

  SOUL_TEST_RUN(test_clone_with_custom_allocator(vectorIntSrc));
  SOUL_TEST_RUN(test_clone_with_custom_allocator(vectorToSrc));
  SOUL_TEST_RUN(test_clone_with_custom_allocator(vectorListToSrc));
}

TEST_F(TestVectorConstructionWithSourceData, TestMoveConstructor)
{
  auto test_move_constructor = []<typename T>(const soul::Vector<T>& vector_src) {
    soul::Vector<T> vector_src_copy = vector_src.clone();
    soul::Vector<T> vector_dst(std::move(vector_src_copy));
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(vector_dst, vector_src));
  };

  SOUL_TEST_RUN(test_move_constructor(vectorIntSrc));
  SOUL_TEST_RUN(test_move_constructor(vectorToSrc));
  SOUL_TEST_RUN(test_move_constructor(vectorListToSrc));
}

TEST_F(TestVectorConstructionWithSourceData, TestRangeConstruction)
{
  auto test_range_construction = []<typename T>(const soul::Vector<T>& vector_src) {
    auto vector_dst = soul::Vector<T>::from(vector_src | soul::views::duplicate<T>());
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(vector_dst, vector_src));
  };

  SOUL_TEST_RUN(test_range_construction(vectorIntSrc));
  SOUL_TEST_RUN(test_range_construction(vectorToSrc));
  SOUL_TEST_RUN(test_range_construction(vectorListToSrc));
}

TEST_F(TestVectorConstructionWithSourceData, TestRangeConstructionWithAllocator)
{
  auto test_range_construction_with_custom_allocator =
    []<typename T>(const soul::Vector<T>& vector_src) {
      TestAllocator test_allocator;

      auto vector_dst =
        soul::Vector<T>::from(vector_src | soul::views::duplicate<T>(), test_allocator);
      SOUL_TEST_ASSERT_TRUE(std::ranges::equal(vector_src, vector_dst));

      SOUL_TEST_ASSERT_EQ(test_allocator.allocCount, 1);
    };

  SOUL_TEST_RUN(test_range_construction_with_custom_allocator(vectorIntSrc));
  SOUL_TEST_RUN(test_range_construction_with_custom_allocator(vectorToSrc));
  SOUL_TEST_RUN(test_range_construction_with_custom_allocator(vectorListToSrc));
}

template <typename T>
auto test_vector_getter(const usize size) -> void
{
  SOUL_TEST_ASSERT_NE(size, 0);
  auto arr = new T[size];
  SCOPE_EXIT(delete[] arr);
  generate_random_array(arr, size);

  const auto middle = size / 2;

  auto vector = soul::Vector<T>::from(soul::views::duplicate_span(arr, size));
  SOUL_TEST_ASSERT_EQ(vector.front(), arr[0]);
  SOUL_TEST_ASSERT_EQ(vector.back(), arr[size - 1]);
  SOUL_TEST_ASSERT_EQ(vector[middle], arr[middle]);
}

TEST(TestVectorGetter, TestVectorGetter)
{
  SOUL_TEST_RUN(test_vector_getter<int>(7));
  SOUL_TEST_RUN(test_vector_getter<TestObject>(9));
  SOUL_TEST_RUN(test_vector_getter<ListTestObject>(7));
}

class TestVectorManipulation : public testing::Test
{
public:
  int intArr[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  TestObject testObjectArr[5] = {
    TestObject(1), TestObject(2), TestObject(3), TestObject(4), TestObject(5)};
  ListTestObject listTestObjectArr[5] = {
    ListTestObject::with_size(1),
    ListTestObject::with_size(2),
    ListTestObject::with_size(3),
    ListTestObject::with_size(4),
    ListTestObject::with_size(5),
  };

  VectorInt vector_int_empty;
  VectorObj vector_testobj_empty;
  VectorListObj vector_list_testobj_empty;

  VectorInt vector_int_arr = VectorInt::from(intArr);
  VectorObj vector_testobj_arr = VectorObj::from(testObjectArr | soul::views::clone<TestObject>());
  VectorListObj vector_list_testobj_arr =
    VectorListObj::from(listTestObjectArr | soul::views::clone<ListTestObject>());
};

TEST_F(TestVectorManipulation, TestVectorSetAllocator)
{
  auto test_vector_set_allocator = []<typename T>(const soul::Vector<T>& sample_vector) {
    TestAllocator test_allocator;
    auto test_vector = sample_vector.clone();
    const auto test_vector_copy = test_vector.clone();
    test_vector.set_allocator(test_allocator);
    SOUL_TEST_ASSERT_EQ(test_vector.get_allocator(), &test_allocator);
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(test_vector, test_vector_copy));
  };

  SOUL_TEST_RUN(test_vector_set_allocator(vector_int_empty));
  SOUL_TEST_RUN(test_vector_set_allocator(vector_testobj_empty));
  SOUL_TEST_RUN(test_vector_set_allocator(vector_list_testobj_empty));

  SOUL_TEST_RUN(test_vector_set_allocator(vector_int_arr));
  SOUL_TEST_RUN(test_vector_set_allocator(vector_testobj_arr));
  SOUL_TEST_RUN(test_vector_set_allocator(vector_list_testobj_arr));
}

TEST_F(TestVectorManipulation, TestVectorCloneFrom)
{
  auto test_assignment_operator =
    []<typename T>(const soul::Vector<T>& sample_vector, const usize size) {
      auto test_vector = sample_vector.clone();
      soul::memory::Allocator* allocator = test_vector.get_allocator();
      auto src_arr = new T[size];
      SCOPE_EXIT(delete[] src_arr);
      auto test_src = soul::Vector<T>::from(soul::views::duplicate_span(src_arr, size));
      test_vector.clone_from(test_src);
      SOUL_TEST_ASSERT_EQ(test_vector.size(), test_src.size());
      SOUL_TEST_ASSERT_TRUE(std::ranges::equal(test_vector, test_src));
      SOUL_TEST_ASSERT_EQ(test_vector.get_allocator(), allocator);
    };

  SOUL_TEST_RUN(test_assignment_operator(vector_int_empty, 5));
  SOUL_TEST_RUN(test_assignment_operator(vector_testobj_empty, 5));
  SOUL_TEST_RUN(test_assignment_operator(vector_list_testobj_empty, 5));

  SOUL_TEST_RUN(test_assignment_operator(vector_int_arr, vector_int_arr.size() + 2));
  SOUL_TEST_RUN(test_assignment_operator(vector_testobj_arr, vector_testobj_arr.size() + 2));
  SOUL_TEST_RUN(
    test_assignment_operator(vector_list_testobj_arr, vector_list_testobj_arr.size() + 2));

  SOUL_TEST_RUN(test_assignment_operator(vector_int_arr, vector_int_arr.size() - 3));
  SOUL_TEST_RUN(test_assignment_operator(vector_testobj_arr, vector_testobj_arr.size() - 3));
  SOUL_TEST_RUN(
    test_assignment_operator(vector_list_testobj_arr, vector_list_testobj_arr.size() - 3));

  TestAllocator test_allocator("Test Allocator For Copy Assignment Operator");
  VectorObj test_different_allocator(&test_allocator);
  SOUL_TEST_RUN(test_assignment_operator(test_different_allocator, 5));
  SOUL_TEST_RUN(test_assignment_operator(test_different_allocator, 7));
}

TEST_F(TestVectorManipulation, TestVectorMoveAssignmentOperator)
{
  auto test_move_assignment_operator =
    []<typename T>(const soul::Vector<T>& sample_vector, const usize size) {
      auto test_vector = sample_vector.clone();
      soul::memory::Allocator* allocator = test_vector.get_allocator();
      auto src_arr = new T[size];
      SCOPE_EXIT(delete[] src_arr);
      auto test_src = soul::Vector<T>::from(soul::views::duplicate_span(src_arr, size));
      test_vector = std::move(test_src);
      SOUL_TEST_ASSERT_EQ(test_vector.size(), size);
      SOUL_TEST_ASSERT_TRUE(
        std::equal(test_vector.begin(), test_vector.end(), src_arr, src_arr + size));
      SOUL_TEST_ASSERT_EQ(test_vector.get_allocator(), allocator);
    };

  SOUL_TEST_RUN(test_move_assignment_operator(vector_int_empty, 5));
  SOUL_TEST_RUN(test_move_assignment_operator(vector_testobj_empty, 5));
  SOUL_TEST_RUN(test_move_assignment_operator(vector_list_testobj_empty, 5));

  SOUL_TEST_RUN(test_move_assignment_operator(vector_int_arr, vector_int_arr.size() + 2));
  SOUL_TEST_RUN(test_move_assignment_operator(vector_testobj_arr, vector_testobj_arr.size() + 2));
  SOUL_TEST_RUN(
    test_move_assignment_operator(vector_list_testobj_arr, vector_list_testobj_arr.size() + 2));

  SOUL_TEST_RUN(test_move_assignment_operator(vector_int_arr, vector_int_arr.size() - 3));
  SOUL_TEST_RUN(test_move_assignment_operator(vector_testobj_arr, vector_testobj_arr.size() - 3));
  SOUL_TEST_RUN(
    test_move_assignment_operator(vector_list_testobj_arr, vector_list_testobj_arr.size() - 3));

  TestAllocator test_allocator("Test Allocator For Move Assignment Operator");
  VectorObj test_different_allocator(&test_allocator);
  SOUL_TEST_RUN(test_move_assignment_operator(test_different_allocator, 5));
  SOUL_TEST_RUN(test_move_assignment_operator(test_different_allocator, 7));
}

TEST_F(TestVectorManipulation, TestVectorAssignWithSizeAndValue)
{
  static constexpr usize ASSIGN_VECTOR_SIZE = 5;
  static constexpr auto ASSIGN_VECTOR_DEFAULT_VALUE = 8;

  auto test_assign_with_size_and_value =
    []<typename T>(const soul::Vector<T>& sample_vector, const usize size, const T& val) {
      auto test_vector = sample_vector.clone();
      test_vector.assign(size, val);
      SOUL_TEST_ASSERT_EQ(test_vector.size(), size);
      SOUL_TEST_ASSERT_TRUE(all_equal(test_vector, val));
    };

  SOUL_TEST_RUN(test_assign_with_size_and_value(
    vector_int_empty, ASSIGN_VECTOR_SIZE, ASSIGN_VECTOR_DEFAULT_VALUE));

  static constexpr usize ASSIGN_VECTOR_OFFSET_SIZE = 2;
  SOUL_TEST_RUN(test_assign_with_size_and_value(
    vector_int_arr,
    vector_int_arr.size() + ASSIGN_VECTOR_OFFSET_SIZE,
    ASSIGN_VECTOR_DEFAULT_VALUE));
  SOUL_TEST_RUN(test_assign_with_size_and_value(
    vector_int_arr,
    vector_int_arr.size() - ASSIGN_VECTOR_OFFSET_SIZE,
    ASSIGN_VECTOR_DEFAULT_VALUE));
}

TEST_F(TestVectorManipulation, TestVectorAssignRange)
{
  auto test_assignment = []<typename T>(soul::Vector<T>& vector, usize size) {
    T* arr = new T[size];
    SCOPE_EXIT(delete[] arr);
    generate_random_array(arr, size);
    vector.assign(soul::views::duplicate_span(arr, size));
    SOUL_TEST_ASSERT_EQ(vector.size(), size);
    SOUL_TEST_ASSERT_TRUE(std::equal(vector.begin(), vector.end(), arr, arr + size));
  };

  SOUL_TEST_RUN(test_assignment(vector_int_empty, 4));
  SOUL_TEST_RUN(test_assignment(vector_testobj_empty, 4));
  SOUL_TEST_RUN(test_assignment(vector_list_testobj_empty, 4));

  SOUL_TEST_RUN(test_assignment(vector_int_arr, vector_int_arr.size() + 5));
  SOUL_TEST_RUN(test_assignment(vector_testobj_arr, vector_testobj_arr.size() + 5));
  SOUL_TEST_RUN(test_assignment(vector_list_testobj_empty, vector_list_testobj_arr.size() + 5));

  SOUL_TEST_RUN(test_assignment(vector_int_arr, vector_int_arr.size() - 2));
  SOUL_TEST_RUN(test_assignment(vector_testobj_arr, vector_testobj_arr.size() - 2));
  SOUL_TEST_RUN(test_assignment(vector_list_testobj_arr, vector_list_testobj_arr.size() - 2));
}

TEST_F(TestVectorManipulation, TestVectorSwap)
{
  auto test_swap = []<typename T>(soul::Vector<T>& test_vector, usize size) {
    T* arr = new T[size];
    SCOPE_EXIT(delete[] arr);
    generate_random_array(arr, size);
    auto swapped_vector = soul::Vector<T>::from(soul::views::duplicate_span(arr, size));
    soul::Vector<T> test_copy = test_vector.clone();

    test_vector.swap(swapped_vector);
    SOUL_TEST_ASSERT_EQ(test_vector.size(), size);
    SOUL_TEST_ASSERT_EQ(swapped_vector.size(), test_copy.size());
    SOUL_TEST_ASSERT_TRUE(std::equal(test_vector.begin(), test_vector.end(), arr, arr + size));
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(swapped_vector, test_copy));

    swap(test_vector, swapped_vector);
    SOUL_TEST_ASSERT_EQ(swapped_vector.size(), size);
    SOUL_TEST_ASSERT_EQ(test_vector.size(), test_copy.size());
    SOUL_TEST_ASSERT_TRUE(
      std::equal(swapped_vector.begin(), swapped_vector.end(), arr, arr + size));
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(test_vector, test_copy));
  };

  SOUL_TEST_RUN(test_swap(vector_int_empty, 5));
  SOUL_TEST_RUN(test_swap(vector_testobj_empty, 5));
  SOUL_TEST_RUN(test_swap(vector_list_testobj_empty, 5));

  SOUL_TEST_RUN(test_swap(vector_int_arr, 5));
  SOUL_TEST_RUN(test_swap(vector_testobj_arr, 5));
  SOUL_TEST_RUN(test_swap(vector_list_testobj_arr, 5));
}

TEST_F(TestVectorManipulation, TestVectorResize)
{
  auto test_resize = []<typename T>(const soul::Vector<T>& sample_vector, const usize size) {
    auto test_vector = sample_vector.clone();
    soul::Vector<T> test_copy = test_vector.clone();
    if constexpr (std::same_as<T, TestObject> || std::same_as<T, ListTestObject>) {
      TestObject::reset();
    }
    test_vector.resize(size);
    SOUL_TEST_ASSERT_EQ(test_vector.size(), size);
    usize smaller_size = std::min(test_vector.size(), test_copy.size());
    SOUL_TEST_ASSERT_TRUE(
      std::equal(test_vector.begin(), test_vector.begin() + smaller_size, test_copy.begin()));
    if (size > test_copy.size()) {
      SOUL_TEST_ASSERT_TRUE(
        std::all_of(test_vector.begin() + smaller_size, test_vector.end(), [](const T& x) {
          return x == T();
        }));
    } else {
      if constexpr (std::same_as<T, TestObject>) {
        SOUL_TEST_ASSERT_EQ(
          TestObject::sTODtorCount - TestObject::sTOCtorCount, test_copy.size() - size);
      }
      if constexpr (std::same_as<T, ListTestObject>) {
        const auto destructed_objects_count = std::accumulate(
          test_copy.begin() + size,
          test_copy.end(),
          usize(0),
          [](const usize prev, const ListTestObject& curr) { return prev + curr.size(); });
        SOUL_TEST_ASSERT_EQ(
          TestObject::sTODtorCount - TestObject::sTOCtorCount, destructed_objects_count);
      }
    }
  };

  SOUL_TEST_RUN(test_resize(vector_int_empty, 5));
  SOUL_TEST_RUN(test_resize(vector_int_arr, vector_int_arr.size() + 2));
  SOUL_TEST_RUN(test_resize(vector_int_arr, vector_int_arr.size() - 3));
  SOUL_TEST_RUN(test_resize(vector_int_arr, 0));

  SOUL_TEST_RUN(test_resize(vector_testobj_empty, 5));
  SOUL_TEST_RUN(test_resize(vector_testobj_arr, vector_testobj_arr.size() + 2));
  SOUL_TEST_RUN(test_resize(vector_testobj_arr, vector_testobj_arr.size() - 3));
  SOUL_TEST_RUN(test_resize(vector_testobj_arr, 0));

  SOUL_TEST_RUN(test_resize(vector_list_testobj_empty, 5));
  SOUL_TEST_RUN(test_resize(vector_testobj_arr, vector_testobj_arr.size() + 2));
  SOUL_TEST_RUN(test_resize(vector_list_testobj_arr, vector_list_testobj_arr.size() - 3));
  SOUL_TEST_RUN(test_resize(vector_list_testobj_arr, 0));
}

TEST_F(TestVectorManipulation, TestVectorReserve)
{
  auto test_reserve = []<typename T>(soul::Vector<T>& test_vector, const usize new_capacity) {
    const auto old_capacity = test_vector.capacity();
    soul::Vector<T> test_copy = test_vector.clone();
    test_vector.reserve(new_capacity);
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(test_vector, test_copy));
    if (old_capacity >= new_capacity) {
      SOUL_TEST_ASSERT_EQ(test_vector.capacity(), old_capacity);
    } else {
      SOUL_TEST_ASSERT_EQ(test_vector.capacity(), new_capacity);
    }
  };

  SOUL_TEST_RUN(test_reserve(vector_int_empty, 5));
  SOUL_TEST_RUN(test_reserve(vector_testobj_empty, 5));
  SOUL_TEST_RUN(test_reserve(vector_list_testobj_empty, 5));

  SOUL_TEST_RUN(test_reserve(vector_int_arr, vector_int_arr.capacity() + 3));
  SOUL_TEST_RUN(test_reserve(vector_testobj_arr, vector_testobj_arr.capacity() + 5));
  SOUL_TEST_RUN(test_reserve(vector_list_testobj_arr, vector_list_testobj_arr.capacity() + 5));

  SOUL_TEST_RUN(test_reserve(vector_int_arr, vector_int_arr.capacity() - 2));
  SOUL_TEST_RUN(test_reserve(vector_testobj_arr, vector_testobj_arr.capacity() - 2));
  SOUL_TEST_RUN(test_reserve(vector_list_testobj_arr, vector_list_testobj_arr.capacity() - 2));
}

TEST_F(TestVectorManipulation, TestVectorShrinkToFit)
{
  auto test_shrink_to_fit = []<typename T>(soul::Vector<T>& test_vector, const usize new_capacity) {
    const auto old_capacity = test_vector.capacity();
    soul::Vector<T> test_copy = test_vector.clone();
    test_vector.reserve(new_capacity);
    test_vector.shrink_to_fit();
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(test_vector, test_copy));
    SOUL_TEST_ASSERT_EQ(test_vector.capacity(), test_copy.size());
  };

  SOUL_TEST_RUN(test_shrink_to_fit(vector_int_empty, 5));
  SOUL_TEST_RUN(test_shrink_to_fit(vector_testobj_empty, 5));
  SOUL_TEST_RUN(test_shrink_to_fit(vector_list_testobj_empty, 5));

  SOUL_TEST_RUN(test_shrink_to_fit(vector_int_arr, vector_int_arr.capacity() + 3));
  SOUL_TEST_RUN(test_shrink_to_fit(vector_testobj_arr, vector_testobj_arr.capacity() + 5));
  SOUL_TEST_RUN(
    test_shrink_to_fit(vector_list_testobj_arr, vector_list_testobj_arr.capacity() + 5));
}

TEST_F(TestVectorManipulation, TestVectorPushBack)
{
  auto test_push_back = []<typename T>(const soul::Vector<T>& sample_vector, const T& val) {
    using Vector = soul::Vector<T>;

    auto test_vector = sample_vector.clone();
    Vector test_copy1 = test_vector.clone();
    Vector test_copy2 = test_vector.clone();

    if constexpr (!soul::ts_clone<T>) {
      test_copy1.push_back(val);
      SOUL_TEST_ASSERT_EQ(test_copy1.size(), test_vector.size() + 1);
      SOUL_TEST_ASSERT_TRUE(std::equal(test_vector.begin(), test_vector.end(), test_copy1.begin()));
      SOUL_TEST_ASSERT_EQ(test_copy1.back(), val);
    }

    T val_copy = soul::duplicate(val);
    test_copy2.push_back(std::move(val_copy));
    SOUL_TEST_ASSERT_EQ(test_copy2.size(), test_vector.size() + 1);
    SOUL_TEST_ASSERT_TRUE(std::equal(test_vector.begin(), test_vector.end(), test_copy2.begin()));
    SOUL_TEST_ASSERT_EQ(test_copy2.back(), val);
  };

  SOUL_TEST_RUN(test_push_back(vector_int_empty, 5));
  SOUL_TEST_RUN(test_push_back(vector_testobj_empty, TestObject(5)));
  SOUL_TEST_RUN(test_push_back(vector_list_testobj_empty, ListTestObject::with_size(5)));

  SOUL_TEST_RUN(test_push_back(vector_int_arr, 5));
  SOUL_TEST_RUN(test_push_back(vector_testobj_arr, TestObject(5)));
  SOUL_TEST_RUN(test_push_back(vector_list_testobj_arr, ListTestObject::with_size(5)));
}

TEST_F(TestVectorManipulation, TestVectorGenerateBack)
{
  auto test_generate_back =
    []<typename T, typename Fn>(const soul::Vector<T>& sample_vector, Fn fn) {
      T val = std::invoke(fn);
      auto test_vector = sample_vector.clone();
      test_vector.generate_back(std::move(fn));
      SOUL_TEST_ASSERT_EQ(test_vector.size(), sample_vector.size() + 1);
      SOUL_TEST_ASSERT_TRUE(
        std::equal(sample_vector.begin(), sample_vector.end(), test_vector.begin()));
      SOUL_TEST_ASSERT_EQ(test_vector.back(), val);
    };

  SOUL_TEST_RUN(test_generate_back(vector_int_empty, [] { return 5; }));
  SOUL_TEST_RUN(test_generate_back(vector_testobj_empty, [] { return TestObject(5); }));
  SOUL_TEST_RUN(
    test_generate_back(vector_list_testobj_empty, [] { return ListTestObject::with_size(5); }));

  SOUL_TEST_RUN(test_generate_back(vector_int_arr, [] { return 5; }));
  SOUL_TEST_RUN(test_generate_back(vector_testobj_arr, [] { return TestObject(5); }));
  SOUL_TEST_RUN(
    test_generate_back(vector_list_testobj_arr, [] { return ListTestObject::with_size(5); }));

  const auto test_object = TestObject(5);
  const auto test_list_object = ListTestObject::with_size(5);
  SOUL_TEST_RUN(test_generate_back(vector_testobj_arr, soul::clone_fn(test_object)));
  SOUL_TEST_RUN(test_generate_back(vector_list_testobj_arr, soul::clone_fn(test_list_object)));
}

TEST_F(TestVectorManipulation, TestVectorPopBack)
{
  auto test_pop_back = []<typename T>(const soul::Vector<T>& sample_vector) {
    auto test_vector = sample_vector.clone();
    soul::Vector<T> test_copy = test_vector.clone();
    if constexpr (std::same_as<T, TestObject>) {
      TestObject::reset();
    }
    test_copy.pop_back();
    SOUL_TEST_ASSERT_EQ(test_copy.size(), test_vector.size() - 1);
    SOUL_TEST_ASSERT_TRUE(std::equal(test_copy.begin(), test_copy.end(), test_vector.cbegin()));
    if constexpr (std::same_as<T, TestObject>) {
      SOUL_TEST_ASSERT_EQ(TestObject::sTODtorCount - TestObject::sTOCtorCount, 1);
    }
  };

  SOUL_TEST_RUN(test_pop_back(vector_int_arr));
  SOUL_TEST_RUN(test_pop_back(vector_testobj_arr));
  SOUL_TEST_RUN(test_pop_back(vector_list_testobj_arr));

  auto test_pop_back_with_size =
    []<typename T>(const soul::Vector<T>& sample_vector, const usize pop_back_size) {
      auto test_vector = sample_vector.clone();
      soul::Vector<T> test_copy = test_vector.clone();
      if constexpr (std::same_as<T, TestObject>) {
        TestObject::reset();
      }
      test_copy.pop_back(pop_back_size);
      SOUL_TEST_ASSERT_EQ(test_copy.size(), test_vector.size() - pop_back_size);
      SOUL_TEST_ASSERT_TRUE(std::equal(test_copy.begin(), test_copy.end(), test_vector.cbegin()));
      if constexpr (std::same_as<T, TestObject>) {
        SOUL_TEST_ASSERT_EQ(TestObject::sTODtorCount - TestObject::sTOCtorCount, pop_back_size);
      }
    };

  SOUL_TEST_RUN(test_pop_back_with_size(vector_int_arr, 3));
  SOUL_TEST_RUN(test_pop_back_with_size(vector_testobj_arr, 3));
  SOUL_TEST_RUN(test_pop_back_with_size(vector_list_testobj_arr, 3));
}

TEST_F(TestVectorManipulation, TestVectorEmplaceBack)
{
  auto test_emplace_back = [](const VectorObj& sample_vector) {
    auto test_vector = sample_vector.clone();
    VectorObj test_copy1 = test_vector.clone();
    test_copy1.emplace_back(3);
    SOUL_TEST_ASSERT_EQ(test_copy1.size(), test_vector.size() + 1);
    SOUL_TEST_ASSERT_EQ(test_copy1.back(), TestObject(3));
    SOUL_TEST_ASSERT_TRUE(
      std::equal(test_vector.cbegin(), test_vector.cend(), test_copy1.cbegin()));

    VectorObj test_copy2 = test_vector.clone();
    test_copy2.emplace_back(4, 5, 6);
    SOUL_TEST_ASSERT_EQ(test_copy2.size(), test_vector.size() + 1);
    SOUL_TEST_ASSERT_EQ(test_copy2.back(), TestObject(4 + 5 + 6));
    SOUL_TEST_ASSERT_TRUE(
      std::equal(test_vector.cbegin(), test_vector.cend(), test_copy2.cbegin()));
  };

  SOUL_TEST_RUN(test_emplace_back(vector_testobj_empty));
  SOUL_TEST_RUN(test_emplace_back(vector_testobj_arr));
}

TEST_F(TestVectorManipulation, TestVectorAppend)
{
  auto test_append = []<typename T>(const soul::Vector<T>& test_vector, const usize append_size) {
    auto src_append_arr = new T[append_size];
    SCOPE_EXIT(delete[] src_append_arr);
    generate_random_array(src_append_arr, append_size);

    using Vector = soul::Vector<T>;

    Vector test_copy1 = test_vector.clone();
    test_copy1.append(soul::views::duplicate_span(src_append_arr, append_size));
    SOUL_TEST_ASSERT_EQ(test_copy1.size(), test_vector.size() + append_size);
    SOUL_TEST_ASSERT_TRUE(std::equal(test_vector.begin(), test_vector.end(), test_copy1.begin()));
    SOUL_TEST_ASSERT_TRUE(std::equal(
      test_copy1.begin() + test_vector.size(),
      test_copy1.end(),
      src_append_arr,
      src_append_arr + append_size));

    Vector append_src_vec = Vector::from(soul::views::duplicate_span(src_append_arr, append_size));
    Vector test_copy2 = test_vector.clone();
    test_copy2.append(append_src_vec | soul::views::duplicate<T>());
    SOUL_TEST_ASSERT_EQ(test_copy2.size(), test_copy1.size());
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(test_copy1, test_copy2));
  };

  SOUL_TEST_RUN(test_append(vector_int_empty, 5));
  SOUL_TEST_RUN(test_append(vector_testobj_empty, 5));
  SOUL_TEST_RUN(test_append(vector_list_testobj_empty, 5));

  SOUL_TEST_RUN(test_append(vector_int_arr, 5));
  SOUL_TEST_RUN(test_append(vector_testobj_arr, 5));
  SOUL_TEST_RUN(test_append(vector_list_testobj_arr, 5));

  SOUL_TEST_RUN(test_append(vector_int_arr, 0));
  SOUL_TEST_RUN(test_append(vector_testobj_arr, 0));
  SOUL_TEST_RUN(test_append(vector_list_testobj_arr, 0));
}

TEST_F(TestVectorManipulation, TestVectorClear)
{
  auto test_clear = []<typename T>(const soul::Vector<T>& sample_vector) {
    auto test_vector = sample_vector.clone();
    const usize old_capacity = test_vector.capacity();
    const usize old_size = test_vector.size();
    if constexpr (std::same_as<T, TestObject>) {
      TestObject::reset();
    }
    test_vector.clear();
    SOUL_TEST_ASSERT_EQ(test_vector.size(), 0);
    SOUL_TEST_ASSERT_EQ(test_vector.capacity(), old_capacity);
    if constexpr (std::same_as<T, TestObject>) {
      SOUL_TEST_ASSERT_EQ(TestObject::sTODtorCount - TestObject::sTOCtorCount, old_size);
    }
  };
  SOUL_TEST_RUN(test_clear(vector_int_empty));
  SOUL_TEST_RUN(test_clear(vector_testobj_empty));
  SOUL_TEST_RUN(test_clear(vector_list_testobj_empty));

  SOUL_TEST_RUN(test_clear(vector_int_arr));
  SOUL_TEST_RUN(test_clear(vector_testobj_arr));
  SOUL_TEST_RUN(test_clear(vector_list_testobj_arr));
}

TEST_F(TestVectorManipulation, TestVectorCleanup)
{
  auto test_cleanup = []<typename T>(const soul::Vector<T>& sample_vector) {
    auto test_vector = sample_vector.clone();
    const usize old_size = test_vector.size();
    if constexpr (std::same_as<T, TestObject>) {
      TestObject::reset();
    }
    test_vector.cleanup();
    SOUL_TEST_ASSERT_EQ(test_vector.size(), 0);
    SOUL_TEST_ASSERT_EQ(test_vector.capacity(), 0);
    if constexpr (std::same_as<T, TestObject>) {
      SOUL_TEST_ASSERT_EQ(TestObject::sTODtorCount - TestObject::sTOCtorCount, old_size);
    }
  };

  SOUL_TEST_RUN(test_cleanup(vector_int_empty));
  SOUL_TEST_RUN(test_cleanup(vector_testobj_empty));
  SOUL_TEST_RUN(test_cleanup(vector_list_testobj_empty));

  SOUL_TEST_RUN(test_cleanup(vector_int_arr));
  SOUL_TEST_RUN(test_cleanup(vector_testobj_arr));
  SOUL_TEST_RUN(test_cleanup(vector_list_testobj_arr));
}
