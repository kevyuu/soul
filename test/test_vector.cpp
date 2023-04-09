#include <algorithm>
#include <array>
#include <list>
#include <numeric>
#include <random>

#include <gtest/gtest.h>

#define USE_CUSTOM_DEFAULT_ALLOCATOR
#include "core/config.h"
#include "core/vector.h"
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

using ListTestObject = std::list<TestObject>;
using VectorInt = soul::Vector<int>;
using VectorObj = soul::Vector<TestObject>;
using VectorListObj = soul::Vector<ListTestObject>;

constexpr soul_size CONSTRUCTOR_VECTOR_SIZE = 10;
constexpr int CONSTRUCTOR_VECTOR_DEFAULT_VALUE = 7;

template <typename T>
static auto generate_random_array(T* arr, soul_size size) -> void
{
  std::random_device random_device;
  std::mt19937 random_engine(random_device());
  std::uniform_int_distribution<int> dist(1, 100);
  for (soul_size i = 0; i < size; i++) {
    arr[i] = T(dist(random_engine));
  }
}

template <typename T>
auto all_equal(soul::Vector<T>& vec, const T& val) -> bool
{
  return std::ranges::all_of(vec, [val](const T& x) { return x == val; });
}

template <typename T, soul_size N>
auto verify_vector(const soul::Vector<T>& vec, const std::array<T, N>& arr) -> bool
{
  return std::equal(vec.begin(), vec.end(), std::begin(arr));
}

template <typename T>
auto test_constructor() -> void
{
  soul::Vector<T> vector;
  SOUL_TEST_ASSERT_TRUE(vector.empty());
}

TEST(TestVectorConstructor, TestDefaultConstructor)
{
  SOUL_TEST_RUN(test_constructor<int>());
  SOUL_TEST_RUN(test_constructor<TestObject>());
  SOUL_TEST_RUN(test_constructor<ListTestObject>());
}

TEST(TestVectorConstructor, TestCustomAllocatorConstructor)
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
auto test_constructor_with_size(const soul_size size) -> void
{
  soul::Vector<T> vector(size);
  SOUL_TEST_ASSERT_EQ(vector.size(), size);
  SOUL_TEST_ASSERT_TRUE(all_equal(vector, T()));
}

TEST(TestVectorConstructor, TestConstructorWithSize)
{
  SOUL_TEST_RUN(test_constructor_with_size<int>(CONSTRUCTOR_VECTOR_SIZE));
  SOUL_TEST_RUN(test_constructor_with_size<TestObject>(CONSTRUCTOR_VECTOR_SIZE));
  SOUL_TEST_RUN(test_constructor_with_size<ListTestObject>(CONSTRUCTOR_VECTOR_SIZE));

  SOUL_TEST_RUN(test_constructor_with_size<int>(0));
  SOUL_TEST_RUN(test_constructor_with_size<TestObject>(0));
  SOUL_TEST_RUN(test_constructor_with_size<ListTestObject>(0));
}

template <typename T>
auto test_constructor_with_size_and_value(const soul_size size, const T& val) -> void
{
  soul::Vector<T> vector(size, val);
  SOUL_TEST_ASSERT_EQ(vector.size(), size);
  SOUL_TEST_ASSERT_TRUE(all_equal(vector, val));
}

TEST(TestVectorConstructor, TestConstrucorWithSizeAndValue)
{
  SOUL_TEST_RUN(test_constructor_with_size_and_value(
    CONSTRUCTOR_VECTOR_SIZE, CONSTRUCTOR_VECTOR_DEFAULT_VALUE));
  SOUL_TEST_RUN(test_constructor_with_size_and_value(
    CONSTRUCTOR_VECTOR_SIZE, TestObject(CONSTRUCTOR_VECTOR_DEFAULT_VALUE)));
  SOUL_TEST_RUN(test_constructor_with_size_and_value(
    CONSTRUCTOR_VECTOR_SIZE, ListTestObject(CONSTRUCTOR_VECTOR_SIZE)));
}

class TestVectorConstructorWithSourceData : public testing::Test
{
public:
  VectorInt vectorIntSrc{CONSTRUCTOR_VECTOR_SIZE, CONSTRUCTOR_VECTOR_DEFAULT_VALUE};
  soul::Vector<TestObject> vectorToSrc{
    CONSTRUCTOR_VECTOR_SIZE, TestObject(CONSTRUCTOR_VECTOR_DEFAULT_VALUE)};
  soul::Vector<ListTestObject> vectorListToSrc{
    CONSTRUCTOR_VECTOR_SIZE, ListTestObject(CONSTRUCTOR_VECTOR_DEFAULT_VALUE)};
};

TEST_F(TestVectorConstructorWithSourceData, TestCopyConstructor)
{
  auto test_copy_constructor = []<typename T>(const soul::Vector<T>& vector_src) {
    soul::Vector<T> vector_dst(vector_src);
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(vector_dst, vector_src));
  };

  SOUL_TEST_RUN(test_copy_constructor(vectorIntSrc));
  SOUL_TEST_RUN(test_copy_constructor(vectorToSrc));
  SOUL_TEST_RUN(test_copy_constructor(vectorListToSrc));
}

TEST_F(TestVectorConstructorWithSourceData, TestCopyConstructorWithCustomAllocator)
{
  auto test_constructor_with_custom_allocator = []<typename T>(const soul::Vector<T>& vector_src) {
    TestAllocator::reset_all();
    TestAllocator test_allocator;

    SOUL_TEST_ASSERT_EQ(test_allocator.allocCount, 0);
    soul::Vector<T> vector_dst(vector_src, test_allocator);
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(vector_src, vector_dst));
    SOUL_TEST_ASSERT_EQ(test_allocator.allocCount, 1);
  };

  SOUL_TEST_RUN(test_constructor_with_custom_allocator(vectorIntSrc));
  SOUL_TEST_RUN(test_constructor_with_custom_allocator(vectorToSrc));
  SOUL_TEST_RUN(test_constructor_with_custom_allocator(vectorListToSrc));
}

TEST_F(TestVectorConstructorWithSourceData, TestMoveConstructor)
{
  auto test_move_constructor = []<typename T>(const soul::Vector<T>& vector_src) {
    soul::Vector<T> vector_src_copy(vector_src);
    soul::Vector<T> vector_dst(std::move(vector_src_copy));
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(vector_dst, vector_src));
  };

  SOUL_TEST_RUN(test_move_constructor(vectorIntSrc));
  SOUL_TEST_RUN(test_move_constructor(vectorToSrc));
  SOUL_TEST_RUN(test_move_constructor(vectorListToSrc));
}

TEST_F(TestVectorConstructorWithSourceData, TestIteratorConstructor)
{
  auto test_iterator_constructor = []<typename T>(const soul::Vector<T>& vector_src) {
    soul::Vector<T> vector_dst(vector_src.begin(), vector_src.end());
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(vector_dst, vector_src));
  };

  SOUL_TEST_RUN(test_iterator_constructor(vectorIntSrc));
  SOUL_TEST_RUN(test_iterator_constructor(vectorToSrc));
  SOUL_TEST_RUN(test_iterator_constructor(vectorListToSrc));
}

TEST_F(TestVectorConstructorWithSourceData, TestIteratorConstructorWithCustomAllocator)
{
  auto test_iterator_constructor_with_custom_allocator =
    []<typename T>(const soul::Vector<T>& vector_src) {
      TestAllocator test_allocator;

      soul::Vector<T> vector_dst(vector_src.begin(), vector_src.end(), test_allocator);
      SOUL_TEST_ASSERT_TRUE(std::ranges::equal(vector_src, vector_dst));

      SOUL_TEST_ASSERT_EQ(test_allocator.allocCount, 1);
    };

  SOUL_TEST_RUN(test_iterator_constructor_with_custom_allocator(vectorIntSrc));
  SOUL_TEST_RUN(test_iterator_constructor_with_custom_allocator(vectorToSrc));
  SOUL_TEST_RUN(test_iterator_constructor_with_custom_allocator(vectorListToSrc));
}

template <typename T>
auto test_vector_getter(const soul_size size) -> void
{
  SOUL_TEST_ASSERT_NE(size, 0);
  auto arr = new T[size];
  SCOPE_EXIT(delete[] arr);
  generate_random_array(arr, size);

  const auto middle = size / 2;

  soul::Vector<T> vector(arr, arr + size);
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
    ListTestObject(1), ListTestObject(2), ListTestObject(3), ListTestObject(4), ListTestObject(5)};

  VectorInt vectorIntEmpty;
  VectorObj vectorTOEmpty;
  VectorListObj vectorListTOEmpty;

  VectorInt vectorIntArr{std::begin(intArr), std::end(intArr)};
  VectorObj vectorTOArr{std::begin(testObjectArr), std::end(testObjectArr)};
  VectorListObj vectorListTOArr{std::begin(listTestObjectArr), std::end(listTestObjectArr)};
};

TEST_F(TestVectorManipulation, TestVectorSetAllocator)
{
  auto test_vector_set_allocator = []<typename T>(soul::Vector<T> test_vector) {
    soul::Vector<T> test_vector_copy(test_vector);
    TestAllocator test_allocator;
    test_vector.set_allocator(test_allocator);
    SOUL_TEST_ASSERT_EQ(test_vector.get_allocator(), &test_allocator);
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(test_vector, test_vector_copy));
  };

  SOUL_TEST_RUN(test_vector_set_allocator(vectorIntEmpty));
  SOUL_TEST_RUN(test_vector_set_allocator(vectorTOEmpty));
  SOUL_TEST_RUN(test_vector_set_allocator(vectorListTOEmpty));

  SOUL_TEST_RUN(test_vector_set_allocator(vectorIntArr));
  SOUL_TEST_RUN(test_vector_set_allocator(vectorTOArr));
  SOUL_TEST_RUN(test_vector_set_allocator(vectorListTOArr));
}

TEST_F(TestVectorManipulation, TestVectorCopyAssignmentOperator)
{
  auto test_assignment_operator =
    []<typename T>(soul::Vector<T> test_vector, const soul_size size) {
      soul::memory::Allocator* allocator = test_vector.get_allocator();
      auto src_arr = new T[size];
      SCOPE_EXIT(delete[] src_arr);
      soul::Vector<T> test_src(src_arr, src_arr + size);
      test_vector = test_src;
      SOUL_TEST_ASSERT_EQ(test_vector.size(), test_src.size());
      SOUL_TEST_ASSERT_TRUE(std::ranges::equal(test_vector, test_src));
      SOUL_TEST_ASSERT_EQ(test_vector.get_allocator(), allocator);
    };

  SOUL_TEST_RUN(test_assignment_operator(vectorIntEmpty, 5));
  SOUL_TEST_RUN(test_assignment_operator(vectorTOEmpty, 5));
  SOUL_TEST_RUN(test_assignment_operator(vectorListTOEmpty, 5));

  SOUL_TEST_RUN(test_assignment_operator(vectorIntArr, vectorIntArr.size() + 2));
  SOUL_TEST_RUN(test_assignment_operator(vectorTOArr, vectorTOArr.size() + 2));
  SOUL_TEST_RUN(test_assignment_operator(vectorListTOArr, vectorListTOArr.size() + 2));

  SOUL_TEST_RUN(test_assignment_operator(vectorIntArr, vectorIntArr.size() - 3));
  SOUL_TEST_RUN(test_assignment_operator(vectorTOArr, vectorTOArr.size() - 3));
  SOUL_TEST_RUN(test_assignment_operator(vectorListTOArr, vectorListTOArr.size() - 3));

  TestAllocator test_allocator("Test Allocator For Copy Assignment Operator");
  VectorObj test_different_allocator(&test_allocator);
  SOUL_TEST_RUN(test_assignment_operator(test_different_allocator, 5));
  SOUL_TEST_RUN(test_assignment_operator(test_different_allocator, 7));
}

TEST_F(TestVectorManipulation, TestVectorMoveAssignmentOperator)
{
  auto test_move_assignment_operator =
    []<typename T>(soul::Vector<T> test_vector, const soul_size size) {
      soul::memory::Allocator* allocator = test_vector.get_allocator();
      auto src_arr = new T[size];
      SCOPE_EXIT(delete[] src_arr);
      soul::Vector<T> test_src(src_arr, src_arr + size);
      test_vector = std::move(test_src);
      SOUL_TEST_ASSERT_EQ(test_vector.size(), size);
      SOUL_TEST_ASSERT_TRUE(
        std::equal(test_vector.begin(), test_vector.end(), src_arr, src_arr + size));
      SOUL_TEST_ASSERT_EQ(test_vector.get_allocator(), allocator);
    };

  SOUL_TEST_RUN(test_move_assignment_operator(vectorIntEmpty, 5));
  SOUL_TEST_RUN(test_move_assignment_operator(vectorTOEmpty, 5));
  SOUL_TEST_RUN(test_move_assignment_operator(vectorListTOEmpty, 5));

  SOUL_TEST_RUN(test_move_assignment_operator(vectorIntArr, vectorIntArr.size() + 2));
  SOUL_TEST_RUN(test_move_assignment_operator(vectorTOArr, vectorTOArr.size() + 2));
  SOUL_TEST_RUN(test_move_assignment_operator(vectorListTOArr, vectorListTOArr.size() + 2));

  SOUL_TEST_RUN(test_move_assignment_operator(vectorIntArr, vectorIntArr.size() - 3));
  SOUL_TEST_RUN(test_move_assignment_operator(vectorTOArr, vectorTOArr.size() - 3));
  SOUL_TEST_RUN(test_move_assignment_operator(vectorListTOArr, vectorListTOArr.size() - 3));

  TestAllocator test_allocator("Test Allocator For Move Assignment Operator");
  VectorObj test_different_allocator(&test_allocator);
  SOUL_TEST_RUN(test_move_assignment_operator(test_different_allocator, 5));
  SOUL_TEST_RUN(test_move_assignment_operator(test_different_allocator, 7));
}

TEST_F(TestVectorManipulation, TestVectorAssignWithSizeAndValue)
{
  static constexpr soul_size ASSIGN_VECTOR_SIZE = 5;
  static constexpr auto ASSIGN_VECTOR_DEFAULT_VALUE = 8;

  auto test_assign_with_size_and_value =
    []<typename T>(soul::Vector<T> test_vector, const soul_size size, const T& val) {
      test_vector.assign(size, val);
      SOUL_TEST_ASSERT_EQ(test_vector.size(), size);
      SOUL_TEST_ASSERT_TRUE(all_equal(test_vector, val));
    };

  SOUL_TEST_RUN(test_assign_with_size_and_value(
    vectorIntEmpty, ASSIGN_VECTOR_SIZE, ASSIGN_VECTOR_DEFAULT_VALUE));
  SOUL_TEST_RUN(test_assign_with_size_and_value(
    vectorTOEmpty, ASSIGN_VECTOR_SIZE, TestObject(ASSIGN_VECTOR_DEFAULT_VALUE)));
  SOUL_TEST_RUN(test_assign_with_size_and_value(
    vectorListTOArr, ASSIGN_VECTOR_SIZE, ListTestObject(ASSIGN_VECTOR_DEFAULT_VALUE)));

  static constexpr soul_size ASSIGN_VECTOR_OFFSET_SIZE = 2;
  SOUL_TEST_RUN(test_assign_with_size_and_value(
    vectorIntArr, vectorIntArr.size() + ASSIGN_VECTOR_OFFSET_SIZE, ASSIGN_VECTOR_DEFAULT_VALUE));
  SOUL_TEST_RUN(test_assign_with_size_and_value(
    vectorIntArr, vectorIntArr.size() - ASSIGN_VECTOR_OFFSET_SIZE, ASSIGN_VECTOR_DEFAULT_VALUE));

  SOUL_TEST_RUN(test_assign_with_size_and_value(
    vectorTOArr,
    vectorTOArr.size() + ASSIGN_VECTOR_OFFSET_SIZE,
    TestObject(ASSIGN_VECTOR_DEFAULT_VALUE)));
  SOUL_TEST_RUN(test_assign_with_size_and_value(
    vectorTOArr,
    vectorTOArr.size() - ASSIGN_VECTOR_OFFSET_SIZE,
    TestObject(ASSIGN_VECTOR_DEFAULT_VALUE)));

  SOUL_TEST_RUN(test_assign_with_size_and_value(
    vectorListTOArr,
    vectorListTOArr.size() + ASSIGN_VECTOR_OFFSET_SIZE,
    ListTestObject(ASSIGN_VECTOR_DEFAULT_VALUE)));
  SOUL_TEST_RUN(test_assign_with_size_and_value(
    vectorListTOArr,
    vectorListTOArr.size() - ASSIGN_VECTOR_OFFSET_SIZE,
    ListTestObject(ASSIGN_VECTOR_DEFAULT_VALUE)));
}

TEST_F(TestVectorManipulation, TestVectorAssignWithIterator)
{
  auto test_assignment = []<typename T>(soul::Vector<T>& vector, soul_size size) {
    T* arr = new T[size];
    SCOPE_EXIT(delete[] arr);
    generate_random_array(arr, size);
    vector.assign(arr, arr + size);
    SOUL_TEST_ASSERT_EQ(vector.size(), size);
    SOUL_TEST_ASSERT_TRUE(std::equal(vector.begin(), vector.end(), arr, arr + size));
  };

  SOUL_TEST_RUN(test_assignment(vectorIntEmpty, 4));
  SOUL_TEST_RUN(test_assignment(vectorTOEmpty, 4));
  SOUL_TEST_RUN(test_assignment(vectorListTOEmpty, 4));

  SOUL_TEST_RUN(test_assignment(vectorIntArr, vectorIntArr.size() + 5));
  SOUL_TEST_RUN(test_assignment(vectorTOArr, vectorTOArr.size() + 5));
  SOUL_TEST_RUN(test_assignment(vectorListTOEmpty, vectorListTOArr.size() + 5));

  SOUL_TEST_RUN(test_assignment(vectorIntArr, vectorIntArr.size() - 2));
  SOUL_TEST_RUN(test_assignment(vectorTOArr, vectorTOArr.size() - 2));
  SOUL_TEST_RUN(test_assignment(vectorListTOArr, vectorListTOArr.size() - 2));
}

TEST_F(TestVectorManipulation, TestVectorSwap)
{
  auto test_swap = []<typename T>(soul::Vector<T>& test_vector, soul_size size) {
    T* arr = new T[size];
    SCOPE_EXIT(delete[] arr);
    generate_random_array(arr, size);
    soul::Vector<T> swapped_vector(arr, arr + size);
    soul::Vector<T> test_copy(test_vector);

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

  SOUL_TEST_RUN(test_swap(vectorIntEmpty, 5));
  SOUL_TEST_RUN(test_swap(vectorTOEmpty, 5));
  SOUL_TEST_RUN(test_swap(vectorListTOEmpty, 5));

  SOUL_TEST_RUN(test_swap(vectorIntArr, 5));
  SOUL_TEST_RUN(test_swap(vectorTOArr, 5));
  SOUL_TEST_RUN(test_swap(vectorListTOArr, 5));
}

TEST_F(TestVectorManipulation, TestVectorResize)
{
  auto test_resize = []<typename T>(soul::Vector<T> test_vector, const soul_size size) {
    soul::Vector<T> test_copy(test_vector);
    if constexpr (std::same_as<T, TestObject> || std::same_as<T, ListTestObject>) {
      TestObject::reset();
    }
    test_vector.resize(size);
    SOUL_TEST_ASSERT_EQ(test_vector.size(), size);
    soul_size smaller_size = std::min(test_vector.size(), test_copy.size());
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
          0,
          [](const soul_size prev, const ListTestObject& curr) { return prev + curr.size(); });
        SOUL_TEST_ASSERT_EQ(
          TestObject::sTODtorCount - TestObject::sTOCtorCount, destructed_objects_count);
      }
    }
  };

  SOUL_TEST_RUN(test_resize(vectorIntEmpty, 5));
  SOUL_TEST_RUN(test_resize(vectorIntArr, vectorIntArr.size() + 2));
  SOUL_TEST_RUN(test_resize(vectorIntArr, vectorIntArr.size() - 3));
  SOUL_TEST_RUN(test_resize(vectorIntArr, 0));

  SOUL_TEST_RUN(test_resize(vectorTOEmpty, 5));
  SOUL_TEST_RUN(test_resize(vectorTOArr, vectorTOArr.size() + 2));
  SOUL_TEST_RUN(test_resize(vectorTOArr, vectorTOArr.size() - 3));
  SOUL_TEST_RUN(test_resize(vectorTOArr, 0));

  SOUL_TEST_RUN(test_resize(vectorListTOEmpty, 5));
  SOUL_TEST_RUN(test_resize(vectorTOArr, vectorTOArr.size() + 2));
  SOUL_TEST_RUN(test_resize(vectorListTOArr, vectorListTOArr.size() - 3));
  SOUL_TEST_RUN(test_resize(vectorListTOArr, 0));
}

TEST_F(TestVectorManipulation, TestVectorReserve)
{
  auto test_reserve = []<typename T>(soul::Vector<T>& test_vector, const soul_size new_capacity) {
    const auto old_capacity = test_vector.capacity();
    soul::Vector<T> test_copy(test_vector);
    test_vector.reserve(new_capacity);
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(test_vector, test_copy));
    if (old_capacity >= new_capacity) {
      SOUL_TEST_ASSERT_EQ(test_vector.capacity(), old_capacity);
    } else {
      SOUL_TEST_ASSERT_EQ(test_vector.capacity(), new_capacity);
    }
  };

  SOUL_TEST_RUN(test_reserve(vectorIntEmpty, 5));
  SOUL_TEST_RUN(test_reserve(vectorTOEmpty, 5));
  SOUL_TEST_RUN(test_reserve(vectorListTOEmpty, 5));

  SOUL_TEST_RUN(test_reserve(vectorIntArr, vectorIntArr.capacity() + 3));
  SOUL_TEST_RUN(test_reserve(vectorTOArr, vectorTOArr.capacity() + 5));
  SOUL_TEST_RUN(test_reserve(vectorListTOArr, vectorListTOArr.capacity() + 5));

  SOUL_TEST_RUN(test_reserve(vectorIntArr, vectorIntArr.capacity() - 2));
  SOUL_TEST_RUN(test_reserve(vectorTOArr, vectorTOArr.capacity() - 2));
  SOUL_TEST_RUN(test_reserve(vectorListTOArr, vectorListTOArr.capacity() - 2));
}

TEST_F(TestVectorManipulation, TestVectorPushBack)
{
  auto test_push_back = []<typename T>(soul::Vector<T> test_vector, const T& val) {
    using Vector = soul::Vector<T>;

    Vector test_copy1(test_vector);
    Vector test_copy2(test_vector);
    Vector test_copy3(test_vector);

    test_copy1.push_back(val);
    SOUL_TEST_ASSERT_EQ(test_copy1.size(), test_vector.size() + 1);
    SOUL_TEST_ASSERT_TRUE(std::equal(test_vector.begin(), test_vector.end(), test_copy1.begin()));
    SOUL_TEST_ASSERT_EQ(test_copy1.back(), val);

    T val_copy = val;
    test_copy2.push_back(std::move(val_copy));
    SOUL_TEST_ASSERT_EQ(test_copy2.size(), test_vector.size() + 1);
    SOUL_TEST_ASSERT_TRUE(std::equal(test_vector.begin(), test_vector.end(), test_copy2.begin()));
    SOUL_TEST_ASSERT_EQ(test_copy2.back(), val);

    test_copy3.push_back();
    SOUL_TEST_ASSERT_EQ(test_copy3.size(), test_vector.size() + 1);
    SOUL_TEST_ASSERT_TRUE(std::equal(test_vector.begin(), test_vector.end(), test_copy3.begin()));
    SOUL_TEST_ASSERT_EQ(test_copy3.back(), T());
  };

  SOUL_TEST_RUN(test_push_back(vectorIntEmpty, 5));
  SOUL_TEST_RUN(test_push_back(vectorTOEmpty, TestObject(5)));
  SOUL_TEST_RUN(test_push_back(vectorListTOEmpty, ListTestObject(5)));

  SOUL_TEST_RUN(test_push_back(vectorIntArr, 5));
  SOUL_TEST_RUN(test_push_back(vectorTOEmpty, TestObject(5)));
  SOUL_TEST_RUN(test_push_back(vectorListTOEmpty, ListTestObject(5)));
}

TEST_F(TestVectorManipulation, TestVectorPopBack)
{
  auto test_pop_back = []<typename T>(soul::Vector<T> test_vector) {
    soul::Vector<T> test_copy(test_vector);
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

  SOUL_TEST_RUN(test_pop_back(vectorIntArr));
  SOUL_TEST_RUN(test_pop_back(vectorTOArr));
  SOUL_TEST_RUN(test_pop_back(vectorListTOArr));

  auto test_pop_back_with_size =
    []<typename T>(soul::Vector<T> test_vector, const soul_size pop_back_size) {
      soul::Vector<T> test_copy(test_vector);
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

  SOUL_TEST_RUN(test_pop_back_with_size(vectorIntArr, 3));
  SOUL_TEST_RUN(test_pop_back_with_size(vectorTOArr, 3));
  SOUL_TEST_RUN(test_pop_back_with_size(vectorListTOArr, 3));
}

TEST_F(TestVectorManipulation, TestVectorEmplaceBack)
{
  auto test_emplace_back = [](const VectorObj test_vector) {
    VectorObj test_copy1(test_vector);
    test_copy1.emplace_back(3);
    SOUL_TEST_ASSERT_EQ(test_copy1.size(), test_vector.size() + 1);
    SOUL_TEST_ASSERT_EQ(test_copy1.back(), TestObject(3));
    SOUL_TEST_ASSERT_TRUE(
      std::equal(test_vector.cbegin(), test_vector.cend(), test_copy1.cbegin()));

    VectorObj test_copy2(test_vector);
    test_copy2.emplace_back(4, 5, 6);
    SOUL_TEST_ASSERT_EQ(test_copy2.size(), test_vector.size() + 1);
    SOUL_TEST_ASSERT_EQ(test_copy2.back(), TestObject(4 + 5 + 6));
    SOUL_TEST_ASSERT_TRUE(
      std::equal(test_vector.cbegin(), test_vector.cend(), test_copy2.cbegin()));
  };

  SOUL_TEST_RUN(test_emplace_back(vectorTOEmpty));
  SOUL_TEST_RUN(test_emplace_back(vectorTOArr));
}

TEST_F(TestVectorManipulation, TestVectorAppend)
{
  auto test_append = []<typename T>(soul::Vector<T> test_vector, const soul_size append_size) {
    auto src_append_arr = new T[append_size];
    SCOPE_EXIT(delete[] src_append_arr);
    generate_random_array(src_append_arr, append_size);

    using Vector = soul::Vector<T>;

    Vector test_copy1(test_vector);
    test_copy1.append(src_append_arr, src_append_arr + append_size);
    SOUL_TEST_ASSERT_EQ(test_copy1.size(), test_vector.size() + append_size);
    SOUL_TEST_ASSERT_TRUE(std::equal(test_vector.begin(), test_vector.end(), test_copy1.begin()));
    SOUL_TEST_ASSERT_TRUE(std::equal(
      test_copy1.begin() + test_vector.size(),
      test_copy1.end(),
      src_append_arr,
      src_append_arr + append_size));

    Vector append_src_vec(src_append_arr, src_append_arr + append_size);
    Vector test_copy2(test_vector);
    test_copy2.append(append_src_vec);
    SOUL_TEST_ASSERT_EQ(test_copy2.size(), test_copy1.size());
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(test_copy1, test_copy2));
  };

  SOUL_TEST_RUN(test_append(vectorIntEmpty, 5));
  SOUL_TEST_RUN(test_append(vectorTOEmpty, 5));
  SOUL_TEST_RUN(test_append(vectorListTOEmpty, 5));

  SOUL_TEST_RUN(test_append(vectorIntArr, 5));
  SOUL_TEST_RUN(test_append(vectorTOArr, 5));
  SOUL_TEST_RUN(test_append(vectorListTOArr, 5));

  SOUL_TEST_RUN(test_append(vectorIntArr, 0));
  SOUL_TEST_RUN(test_append(vectorTOArr, 0));
  SOUL_TEST_RUN(test_append(vectorListTOArr, 0));
}

TEST_F(TestVectorManipulation, TestVectorClear)
{
  auto test_clear = []<typename T>(soul::Vector<T> test_vector) {
    const soul_size old_capacity = test_vector.capacity();
    const soul_size old_size = test_vector.size();
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
  SOUL_TEST_RUN(test_clear(vectorIntEmpty));
  SOUL_TEST_RUN(test_clear(vectorTOEmpty));
  SOUL_TEST_RUN(test_clear(vectorListTOEmpty));

  SOUL_TEST_RUN(test_clear(vectorIntArr));
  SOUL_TEST_RUN(test_clear(vectorTOArr));
  SOUL_TEST_RUN(test_clear(vectorListTOArr));
}

TEST_F(TestVectorManipulation, TestVectorCleanup)
{
  auto test_cleanup = []<typename T>(soul::Vector<T> test_vector) {
    const soul_size old_size = test_vector.size();
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

  SOUL_TEST_RUN(test_cleanup(vectorIntEmpty));
  SOUL_TEST_RUN(test_cleanup(vectorTOEmpty));
  SOUL_TEST_RUN(test_cleanup(vectorListTOEmpty));

  SOUL_TEST_RUN(test_cleanup(vectorIntArr));
  SOUL_TEST_RUN(test_cleanup(vectorTOArr));
  SOUL_TEST_RUN(test_cleanup(vectorListTOArr));
}

TEST(TestVectorConstructurWithArray, TestVectorConstructorWithArray)
{
  const soul::Vector<int> vec1 = std::array<int, 0>{};
  SOUL_TEST_ASSERT_EQ(vec1.size(), 0);

  const soul::Vector<int> vec2 = std::array{5, 7, 1};
  SOUL_TEST_ASSERT_EQ(vec2.size(), 3);
  SOUL_TEST_ASSERT_TRUE(verify_vector(vec2, std::array{5, 7, 1}));

  TestObject::reset();
  const soul::Vector<TestObject> vec3 = std::array{TestObject(3), TestObject(7), TestObject(9)};
  SOUL_TEST_ASSERT_EQ(vec3.size(), 3);
  SOUL_TEST_ASSERT_EQ(TestObject::sTOMoveCtorCount, 3);
  SOUL_TEST_ASSERT_TRUE(
    verify_vector(vec3, std::array{TestObject(3), TestObject(7), TestObject(9)}));
}
