#include "core/array.h"
#include "core/bit_vector.h"
#include "core/config.h"
#include "core/sbo_vector.h"
#include "core/type_traits.h"
#include "core/util.h"

#include <xutility>
#include "util.h"

namespace soul
{
  auto get_default_allocator() -> memory::Allocator*
  {
    static TestAllocator test_allocator("Test default allocator");
    return &test_allocator;
  }
} // namespace soul

template <typename T, usize element_count, std::ranges::sized_range RangeT>
auto verify_array(const soul::Array<T, element_count>& arr, RangeT&& range)
{
  SOUL_TEST_ASSERT_EQ(arr.size(), range.size());
  SOUL_TEST_ASSERT_EQ(arr.empty(), range.empty());
  if constexpr (element_count > 0)
  {
    if (!range.empty())
    {
      SOUL_TEST_ASSERT_EQ(arr.front(), *range.begin());
      SOUL_TEST_ASSERT_EQ(arr.back(), *(range.end() - 1));
    }
  }
  SOUL_TEST_ASSERT_TRUE(std::equal(range.begin(), range.end(), arr.begin()));
}

TEST(TestArrayConstruction, TestConstructFromBraceInitList)
{
  const auto array = soul::Array{1, 2, -3};
  SOUL_TEST_ASSERT_EQ(array[0], 1);
  SOUL_TEST_ASSERT_EQ(array[1], 2);
  SOUL_TEST_ASSERT_EQ(array[2], -3);
  SOUL_TEST_ASSERT_EQ(array.cspan<u32>().size(), 3);
};

TEST(TestArrayConstruction, TestDefaultConstructor)
{
  auto test_default_constructor = []<typename T, usize element_count>()
  {
    const soul::Array<T, element_count> arr;
    verify_array<T, element_count>(arr, generate_sequence(element_count, T()));
  };

  SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_default_constructor.operator()<TestObject, 0>()));
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_default_constructor.operator()<TestObject, 4>()));
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_default_constructor.operator()<ListTestObject, 0>()));
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_default_constructor.operator()<ListTestObject, 4>()));
};

TEST(TestArrayConstruction, TestInitFillConstruction)
{
  auto test_init_fill_construction = []<typename T, usize element_count>(const T& val)
  {
    const auto arr = soul::Array<T, element_count>::Fill(val);
    verify_array<T, element_count>(arr, generate_sequence(element_count, val));
  };

  SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_init_fill_construction.operator()<int, 0>(3)));
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_init_fill_construction.operator()<int, 10>(3)));
}

TEST(TestArrayConstruction, TestInitGenerateConstruction)
{
  auto test_init_generate_construction =
    []<typename T, usize element_count>(soul::ts_generate_fn<T> auto fn)
  {
    const auto arr   = soul::Array<T, element_count>::Generate(fn);
    const auto range = std::views::iota(usize(0), element_count) | std::views::transform(
                                                                     [fn](usize /* idx */) -> T
                                                                     {
                                                                       return std::invoke(fn);
                                                                     });
    verify_array<T, element_count>(arr, range);
  };

  auto test_obj_gen_fn = []
  {
    return TestObject(5);
  };
  SOUL_TEST_RUN(
    SOUL_SINGLE_ARG(test_init_generate_construction.operator()<TestObject, 0>(test_obj_gen_fn)));
  SOUL_TEST_RUN(
    SOUL_SINGLE_ARG(test_init_generate_construction.operator()<TestObject, 4>(test_obj_gen_fn)));

  auto list_test_obj_gen_fn = []
  {
    return ListTestObject::WithSize(5);
  };
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(
    test_init_generate_construction.operator()<ListTestObject, 0>(list_test_obj_gen_fn)));
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(
    test_init_generate_construction.operator()<ListTestObject, 4>(list_test_obj_gen_fn)));
}

TEST(TestArrayConstruction, TestInitIndexTransformConstruction)
{
  auto test_init_index_transform_construction =
    []<typename T, usize element_count>(soul::ts_fn<T, usize> auto fn)
  {
    const auto arr   = soul::Array<T, element_count>::TransformIndex(fn);
    const auto range = std::views::iota(usize(0), element_count) | std::views::transform(fn);
    verify_array<T, element_count>(arr, range);
  };

  auto test_obj_gen_fn = [](usize index)
  {
    return TestObject(soul::cast<int>(index));
  };
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(
    test_init_index_transform_construction.operator()<TestObject, 0>(test_obj_gen_fn)));
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(
    test_init_index_transform_construction.operator()<TestObject, 4>(test_obj_gen_fn)));

  auto list_test_obj_gen_fn = [](usize index)
  {
    return ListTestObject::WithSize(index);
  };
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(
    test_init_index_transform_construction.operator()<ListTestObject, 0>(list_test_obj_gen_fn)));
  SOUL_TEST_RUN(SOUL_SINGLE_ARG(
    test_init_index_transform_construction.operator()<ListTestObject, 4>(list_test_obj_gen_fn)));
}

TEST(TestArrayConstruction, TestCopyConstructor)
{
  auto test_copy_constructor = []<typename T, usize element_count>()
  {
    const auto sequence = generate_random_sequence<T>(element_count);
  };
}
