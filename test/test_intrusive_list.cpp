#include <algorithm>
#include <ranges>

#include "core/config.h"
#include "core/intrusive_list.h"
#include "core/util.h"

#include "util.h"

namespace soul
{
  auto get_default_allocator() -> memory::Allocator*
  {
    static TestAllocator test_allocator("Test default allocator"_str);
    return &test_allocator;
  }
} // namespace soul

struct IntNode : public soul::IntrusiveListNode
{
  int x;

  IntNode(int x = 0) : x(x) {}

  operator int() const
  {
    return x;
  }

  auto operator==(const IntNode& rhs) const -> b8
  {
    return x == rhs.x;
  }
};

template <typename T>
auto test_constructor() -> void
{
  soul::IntrusiveList<T> list;
  SOUL_TEST_ASSERT_TRUE(list.empty());
  SOUL_TEST_ASSERT_EQ(list.size(), 0);
}

TEST(TestIntrusiveListConstructor, TestDefaultConstructor)
{
  SOUL_TEST_RUN(test_constructor<IntNode>());
}

template <typename T>
auto verify_sequence(
  soul::IntrusiveList<T>& test_list,
  std::vector<T>& expected_values,
  std::vector<T*>& expected_objects) -> void
{
  SOUL_TEST_ASSERT_EQ(test_list.empty(), expected_values.empty());
  SOUL_TEST_ASSERT_EQ(test_list.size(), expected_values.size());
  if (!expected_values.empty())
  {
    SOUL_TEST_ASSERT_EQ(test_list.front(), expected_values.front());
    SOUL_TEST_ASSERT_EQ(test_list.back(), expected_values.back());
  }

  SOUL_TEST_ASSERT_TRUE(std::equal(
    expected_values.cbegin(), expected_values.cend(), test_list.begin(), test_list.end()));
  SOUL_TEST_ASSERT_TRUE(std::equal(
    expected_values.cbegin(), expected_values.cend(), test_list.cbegin(), test_list.cend()));
  SOUL_TEST_ASSERT_TRUE(std::equal(
    expected_values.crbegin(), expected_values.crend(), test_list.rbegin(), test_list.rend()));
  SOUL_TEST_ASSERT_TRUE(std::equal(
    expected_values.crbegin(), expected_values.crend(), test_list.crbegin(), test_list.crend()));
  SOUL_TEST_ASSERT_TRUE(std::all_of(
    expected_objects.cbegin(),
    expected_objects.cend(),
    [&test_list](const T* val)
    {
      return test_list.contains(*val);
    }));
  for (usize obj_idx = 0; obj_idx < expected_objects.size(); obj_idx++)
  {
    auto location = test_list.locate(*expected_objects[obj_idx]);
    SOUL_TEST_ASSERT_EQ(std::distance(test_list.begin(), location), obj_idx);
    auto const_location = test_list.locate(*static_cast<const T*>(expected_objects[obj_idx]));
    SOUL_TEST_ASSERT_EQ(std::distance(test_list.cbegin(), const_location), obj_idx);
  }
}

TEST(TestIntrusiveListPushBack, TestIntrusiveListPushBack)
{
  auto test_push_back =
    []<typename T>(soul::IntrusiveList<T>& test_list, std::vector<T>& new_values)
  {
    SOUL_TEST_ASSERT_NE(new_values.size(), 0);
    std::vector<T> expected_values(test_list.cbegin(), test_list.cend());
    expected_values.insert(expected_values.end(), new_values.cbegin(), new_values.cend());

    std::vector<T*> expected_objects;
    expected_objects.reserve(expected_values.size());
    for (T& val : test_list)
    {
      expected_objects.push_back(&val);
    }
    for (T& val : new_values)
    {
      expected_objects.push_back(&val);
    }

    for (T& val : new_values)
    {
      test_list.push_back(val);
    }
    verify_sequence(test_list, expected_values, expected_objects);
  };

  soul::IntrusiveList<IntNode> list;
  auto vec_val_single = generate_random_sequence<IntNode>(1);
  SOUL_TEST_RUN(test_push_back(list, vec_val_single));

  static constexpr usize MULTI_PUSH_BACK_COUNT = 30;
  auto vec_val_multi = generate_random_sequence<IntNode>(MULTI_PUSH_BACK_COUNT);
  SOUL_TEST_RUN(test_push_back(list, vec_val_multi));
}

TEST(TestIntrusiveListPushFront, TestIntrusiveListPushFront)
{
  auto test_push_front =
    []<typename T>(soul::IntrusiveList<T>& test_list, std::vector<T>& new_values)
  {
    SOUL_TEST_ASSERT_NE(new_values.size(), 0);
    std::vector<T> expected_values(new_values.crbegin(), new_values.crend());
    expected_values.insert(expected_values.end(), test_list.cbegin(), test_list.cend());

    std::vector<T*> expected_objects;
    expected_objects.reserve(expected_values.size());
    for (T& val : new_values | std::views::reverse)
    {
      expected_objects.push_back(&val);
    }
    for (T& val : test_list)
    {
      expected_objects.push_back(&val);
    }

    for (T& val : new_values)
    {
      test_list.push_front(val);
    }
    verify_sequence(test_list, expected_values, expected_objects);
  };

  soul::IntrusiveList<IntNode> list;
  auto vec_val_single = generate_random_sequence<IntNode>(1);
  SOUL_TEST_RUN(test_push_front(list, vec_val_single));

  static constexpr usize MULTI_PUSH_FRONT_COUNT = 10;
  auto vec_val_multi = generate_random_sequence<IntNode>(MULTI_PUSH_FRONT_COUNT);
  SOUL_TEST_RUN(test_push_front(list, vec_val_multi));
}

template <typename T>
auto generate_random_intrusive_list(
  soul::IntrusiveList<T>& list, std::vector<T>& vec, const usize size) -> void
{
  vec = generate_random_sequence<T>(size);
  for (T& val : vec)
  {
    list.push_back(val);
  }
}

template <typename T>
auto to_pointer(T& val) -> T*
{
  return &val;
}

template <typename T>
auto get_vector_values(const std::vector<T*> objects) -> std::vector<T>
{
  std::vector<T> result;
  result.reserve(objects.size());
  std::transform(
    objects.begin(),
    objects.end(),
    std::back_inserter(result),
    [](T* val) -> T&
    {
      return *val;
    });
  return result;
}

template <typename T>
struct RandomIntrusiveList
{
  soul::IntrusiveList<T> list;
  std::vector<T> pool;

  RandomIntrusiveList() {}

  explicit RandomIntrusiveList(const usize size)
  {
    generate_random_intrusive_list(list, pool, size);
  }
};

TEST(TestIntrusiveListPopBack, TestIntrusiveListPopBack)
{
  auto test_pop_front = []<typename T>(soul::IntrusiveList<T>& test_list)
  {
    SOUL_TEST_ASSERT_FALSE(test_list.empty());
    std::vector<T> expected_values(test_list.begin(), test_list.end());
    expected_values.pop_back();

    std::vector<T*> expected_objects;
    expected_objects.reserve(test_list.size());
    for (T& val : test_list)
    {
      expected_objects.push_back(&val);
    }
    expected_objects.pop_back();

    test_list.pop_back();

    verify_sequence(test_list, expected_values, expected_objects);
  };

  soul::IntrusiveList<IntNode> list;
  std::vector<IntNode> objects;
  generate_random_intrusive_list(list, objects, 10);
  SOUL_TEST_RUN(test_pop_front(list));

  soul::IntrusiveList<IntNode> list2;
  std::vector<IntNode> objects2;
  generate_random_intrusive_list(list2, objects2, 1);
  SOUL_TEST_RUN(test_pop_front(list));
};

TEST(TestIntrusiveListPopFront, TestIntrusiveListPopFront)
{
  auto test_pop_back = []<typename T>(soul::IntrusiveList<T>& test_list)
  {
    SOUL_TEST_ASSERT_FALSE(test_list.empty());
    std::vector<T*> expected_objects;
    expected_objects.reserve(test_list.size() - 1);
    for (auto it = std::next(test_list.begin(), 1); it != test_list.end(); ++it)
    {
      expected_objects.push_back(&(*it));
    }

    std::vector<T> expected_values;
    std::ranges::for_each(
      expected_objects,
      [&expected_values](T* objects)
      {
        expected_values.push_back(*objects);
      });
    test_list.pop_front();
    verify_sequence(test_list, expected_values, expected_objects);
  };

  soul::IntrusiveList<IntNode> list;
  std::vector<IntNode> objects;
  generate_random_intrusive_list(list, objects, 10);
  SOUL_TEST_RUN(test_pop_back(list));

  soul::IntrusiveList<IntNode> list2;
  std::vector<IntNode> objects2;
  generate_random_intrusive_list(list2, objects2, 1);
  SOUL_TEST_RUN(test_pop_back(list2));
}

TEST(TestIntrusiveListInsert, TestIntrusiveListInsert)
{
  auto test_insert =
    []<typename T>(soul::IntrusiveList<T>& test_list, usize position, std::vector<T>& new_objects)
  {
    std::vector<T*> expected_objects;
    expected_objects.reserve(test_list.size() + new_objects.size());
    std::transform(
      test_list.begin(),
      std::next(test_list.begin(), position),
      std::back_inserter(expected_objects),
      to_pointer<T>);
    std::transform(
      new_objects.begin(), new_objects.end(), std::back_inserter(expected_objects), to_pointer<T>);
    std::transform(
      std::next(test_list.begin(), position),
      test_list.end(),
      std::back_inserter(expected_objects),
      to_pointer<T>);

    std::vector<T> expected_values = get_vector_values(expected_objects);

    auto insert_pos = std::next(test_list.begin(), position);
    for (T& object : new_objects)
    {
      auto iterator = test_list.insert(insert_pos, object);
      SOUL_TEST_ASSERT_EQ(*iterator, object);
    }
    verify_sequence(test_list, expected_values, expected_objects);
  };

  soul::IntrusiveList<IntNode> list;
  std::vector<IntNode> objects;
  generate_random_intrusive_list(list, objects, 10);
  auto inserted_objects_middle = generate_random_sequence<IntNode>(5);
  SOUL_TEST_RUN(test_insert(list, list.size() / 2, inserted_objects_middle));

  auto inserted_objects_begin = generate_random_sequence<IntNode>(1);
  SOUL_TEST_RUN(test_insert(list, 0, inserted_objects_begin));

  auto inserted_objects_end = generate_random_sequence<IntNode>(1);
  SOUL_TEST_RUN(test_insert(list, list.size(), inserted_objects_end));
}

TEST(TestIntrusiveListRemove, TestIntrusiveListRemove)
{
  auto test_remove = []<typename T>(soul::IntrusiveList<T>& list, usize position)
  {
    std::vector<T*> expected_objects;
    expected_objects.reserve(list.size() - 1);
    std::transform(
      list.begin(),
      std::next(list.begin(), position),
      std::back_inserter(expected_objects),
      to_pointer<T>);
    std::transform(
      std::next(list.begin(), position + 1),
      list.end(),
      std::back_inserter(expected_objects),
      to_pointer<T>);
    std::vector<T> expected_values = get_vector_values(expected_objects);
    list.remove(*std::next(list.begin(), position));

    verify_sequence(list, expected_values, expected_objects);
  };

  RandomIntrusiveList<IntNode> random_list1(10);
  SOUL_TEST_RUN(test_remove(random_list1.list, 0));
  SOUL_TEST_RUN(test_remove(random_list1.list, random_list1.list.size() - 1));
  SOUL_TEST_RUN(test_remove(random_list1.list, random_list1.list.size() / 2));

  RandomIntrusiveList<IntNode> single_random_list(1);
  SOUL_TEST_RUN(test_remove(single_random_list.list, 0));
}

TEST(TestIntrusiveListErase, TestIntrusiveListEraseSingle)
{
  auto test_erase_single = []<typename T>(soul::IntrusiveList<T>& test_list, usize position)
  {
    std::vector<T*> expected_objects;
    expected_objects.reserve(test_list.size() - 1);
    std::transform(
      test_list.begin(),
      std::next(test_list.begin(), position),
      std::back_inserter(expected_objects),
      to_pointer<T>);
    std::transform(
      std::next(test_list.begin(), position + 1),
      test_list.end(),
      std::back_inserter(expected_objects),
      to_pointer<T>);

    std::vector<T> expected_values = get_vector_values(expected_objects);
    test_list.erase(std::next(test_list.begin(), position));
    verify_sequence(test_list, expected_values, expected_objects);
  };

  RandomIntrusiveList<IntNode> random_list(10);
  SOUL_TEST_RUN(test_erase_single(random_list.list, 0));
  SOUL_TEST_RUN(test_erase_single(random_list.list, random_list.list.size() - 1));
  SOUL_TEST_RUN(test_erase_single(random_list.list, random_list.list.size() / 2));

  RandomIntrusiveList<IntNode> single_random_list1(1);
  SOUL_TEST_RUN(test_erase_single(single_random_list1.list, 0));
}

TEST(TestIntrusiveListErase, TestIntrusiveListEraseRange)
{
  auto test_erase_range = []<typename T>(soul::IntrusiveList<T>& test_list, usize first, usize last)
  {
    std::vector<T*> expected_objects;
    expected_objects.reserve(test_list.size() - (last - first));
    std::transform(
      test_list.begin(),
      std::next(test_list.begin(), first),
      std::back_inserter(expected_objects),
      to_pointer<T>);
    std::transform(
      std::next(test_list.begin(), last),
      test_list.end(),
      std::back_inserter(expected_objects),
      to_pointer<T>);
    std::vector<T> expected_values = get_vector_values(expected_objects);
    test_list.erase(std::next(test_list.begin(), first), std::next(test_list.begin(), last));
    verify_sequence(test_list, expected_values, expected_objects);
  };

  RandomIntrusiveList<IntNode> random_list(10);
  SOUL_TEST_RUN(test_erase_range(random_list.list, 5, 10));

  RandomIntrusiveList<IntNode> random_list2(20);
  SOUL_TEST_RUN(test_erase_range(random_list2.list, 0, 20));
}

class TestIntrusiveListSplice : public testing::Test
{
public:
  RandomIntrusiveList<IntNode> random_list1{10};
  RandomIntrusiveList<IntNode> random_list2{10};
  RandomIntrusiveList<IntNode> random_list3{10};
  RandomIntrusiveList<IntNode> random_list4{10};
  RandomIntrusiveList<IntNode> empty_list{};
  RandomIntrusiveList<IntNode> single_list{1};
};

TEST_F(TestIntrusiveListSplice, TestIntrusiveListSpliceValue)
{
  auto test_splice_value = []<typename T>(
                             soul::IntrusiveList<T>& src_list,
                             soul::IntrusiveList<T>& dst_list,
                             usize src_position,
                             usize dst_position)
  {
    SOUL_TEST_ASSERT_NE(std::next(src_list.begin(), src_position), src_list.end());
    std::vector<T*> src_expected_objects;
    src_expected_objects.reserve(src_list.size() - 1);
    std::transform(
      src_list.begin(),
      std::next(src_list.begin(), src_position),
      std::back_inserter(src_expected_objects),
      to_pointer<T>);
    std::transform(
      std::next(src_list.begin(), src_position + 1),
      src_list.end(),
      std::back_inserter(src_expected_objects),
      to_pointer<T>);
    std::vector<T> src_expected_values = get_vector_values(src_expected_objects);

    std::vector<T*> dst_expected_objects;
    std::transform(
      dst_list.begin(),
      std::next(dst_list.begin(), dst_position),
      std::back_inserter(dst_expected_objects),
      to_pointer<T>);
    dst_expected_objects.push_back(&(*std::next(src_list.begin(), src_position)));
    std::transform(
      std::next(dst_list.begin(), dst_position),
      dst_list.end(),
      std::back_inserter(dst_expected_objects),
      to_pointer<T>);
    std::vector<T> dst_expected_values = get_vector_values(dst_expected_objects);

    dst_list.splice(
      std::next(dst_list.begin(), dst_position), *std::next(src_list.begin(), src_position));

    SOUL_TEST_RUN(verify_sequence(src_list, src_expected_values, src_expected_objects));
    SOUL_TEST_RUN(verify_sequence(dst_list, dst_expected_values, dst_expected_objects));
  };

  SOUL_TEST_RUN(test_splice_value(
    random_list1.list,
    random_list2.list,
    random_list1.list.size() / 2,
    random_list2.list.size() / 2));
  SOUL_TEST_RUN(
    test_splice_value(random_list1.list, random_list2.list, 0, random_list1.list.size() / 2));
  SOUL_TEST_RUN(test_splice_value(
    random_list1.list,
    random_list2.list,
    random_list1.list.size() - 1,
    random_list1.list.size() / 2));

  SOUL_TEST_RUN(test_splice_value(random_list1.list, random_list2.list, 0, 0));
  SOUL_TEST_RUN(
    test_splice_value(random_list1.list, random_list2.list, 0, random_list2.list.size()));

  SOUL_TEST_RUN(test_splice_value(random_list1.list, empty_list.list, 0, 0));
  SOUL_TEST_RUN(test_splice_value(single_list.list, random_list2.list, 0, 0));
}

TEST_F(TestIntrusiveListSplice, TestIntrusiveListSpliceList)
{
  auto test_splice_list =
    []<typename T>(
      soul::IntrusiveList<T>& src_list, soul::IntrusiveList<T>& dst_list, usize position)
  {
    std::vector<T*> src_expected_objects;

    std::vector<T> src_expected_values;

    std::vector<T*> dst_expected_objects;
    std::transform(
      dst_list.begin(),
      std::next(dst_list.begin(), position),
      std::back_inserter(dst_expected_objects),
      to_pointer<T>);
    std::transform(
      src_list.begin(), src_list.end(), std::back_inserter(dst_expected_objects), to_pointer<T>);
    std::transform(
      std::next(dst_list.begin(), position),
      dst_list.end(),
      std::back_inserter(dst_expected_objects),
      to_pointer<T>);
    std::vector<T> dst_expected_values = get_vector_values(dst_expected_objects);

    dst_list.splice(std::next(dst_list.begin(), position), src_list);

    SOUL_TEST_RUN(verify_sequence(src_list, src_expected_values, src_expected_objects));
    SOUL_TEST_RUN(verify_sequence(dst_list, dst_expected_values, dst_expected_objects));
  };

  SOUL_TEST_RUN(
    test_splice_list(random_list2.list, random_list1.list, random_list1.list.size() / 2));
  SOUL_TEST_RUN(test_splice_list(random_list3.list, random_list1.list, 0));
  SOUL_TEST_RUN(test_splice_list(random_list4.list, random_list1.list, random_list1.list.size()));
  SOUL_TEST_RUN(test_splice_list(empty_list.list, random_list1.list, 0));
  SOUL_TEST_RUN(test_splice_list(empty_list.list, random_list2.list, 0));
  SOUL_TEST_RUN(test_splice_list(single_list.list, random_list1.list, 0));
  SOUL_TEST_RUN(test_splice_list(random_list1.list, empty_list.list, 0));
}

TEST_F(TestIntrusiveListSplice, TestIntrusiveListSpliceListSingle)
{
  auto test_splice_list_single = []<typename T>(
                                   soul::IntrusiveList<T>& src_list,
                                   soul::IntrusiveList<T>& dst_list,
                                   usize src_position,
                                   usize dst_position)
  {
    SOUL_TEST_ASSERT_NE(std::next(src_list.begin(), src_position), src_list.end());
    std::vector<T*> src_expected_objects;
    src_expected_objects.reserve(src_list.size() - 1);
    std::transform(
      src_list.begin(),
      std::next(src_list.begin(), src_position),
      std::back_inserter(src_expected_objects),
      to_pointer<T>);
    std::transform(
      std::next(src_list.begin(), src_position + 1),
      src_list.end(),
      std::back_inserter(src_expected_objects),
      to_pointer<T>);
    std::vector<T> src_expected_values = get_vector_values(src_expected_objects);

    std::vector<T*> dst_expected_objects;
    std::transform(
      dst_list.begin(),
      std::next(dst_list.begin(), dst_position),
      std::back_inserter(dst_expected_objects),
      to_pointer<T>);
    dst_expected_objects.push_back(&(*std::next(src_list.begin(), src_position)));
    std::transform(
      std::next(dst_list.begin(), dst_position),
      dst_list.end(),
      std::back_inserter(dst_expected_objects),
      to_pointer<T>);
    std::vector<T> dst_expected_values = get_vector_values(dst_expected_objects);

    dst_list.splice(
      std::next(dst_list.begin(), dst_position),
      src_list,
      std::next(src_list.begin(), src_position));

    SOUL_TEST_RUN(verify_sequence(src_list, src_expected_values, src_expected_objects));
    SOUL_TEST_RUN(verify_sequence(dst_list, dst_expected_values, dst_expected_objects));
  };

  SOUL_TEST_RUN(test_splice_list_single(
    random_list1.list,
    random_list2.list,
    random_list1.list.size() / 2,
    random_list2.list.size() / 2));
  SOUL_TEST_RUN(
    test_splice_list_single(random_list1.list, random_list2.list, 0, random_list1.list.size() / 2));
  SOUL_TEST_RUN(test_splice_list_single(
    random_list1.list,
    random_list2.list,
    random_list1.list.size() - 1,
    random_list1.list.size() / 2));

  SOUL_TEST_RUN(test_splice_list_single(random_list1.list, random_list2.list, 0, 0));
  SOUL_TEST_RUN(
    test_splice_list_single(random_list1.list, random_list2.list, 0, random_list2.list.size()));

  SOUL_TEST_RUN(test_splice_list_single(random_list1.list, empty_list.list, 0, 0));
  SOUL_TEST_RUN(test_splice_list_single(single_list.list, random_list2.list, 0, 0));
}

TEST_F(TestIntrusiveListSplice, TestIntrusiveListSpliceRange)
{
  auto test_splice_list_range = []<typename T>(
                                  soul::IntrusiveList<T>& src_list,
                                  soul::IntrusiveList<T>& dst_list,
                                  usize src_position_start,
                                  usize src_position_end,
                                  usize dst_position)
  {
    std::vector<T*> src_expected_objects;
    src_expected_objects.reserve(src_list.size() - (src_position_end - src_position_start));
    std::transform(
      src_list.begin(),
      std::next(src_list.begin(), src_position_start),
      std::back_inserter(src_expected_objects),
      to_pointer<T>);
    std::transform(
      std::next(src_list.begin(), src_position_end),
      src_list.end(),
      std::back_inserter(src_expected_objects),
      to_pointer<T>);
    std::vector<T> src_expected_values = get_vector_values(src_expected_objects);

    std::vector<T*> dst_expected_objects;
    dst_expected_objects.reserve(dst_list.size() + src_position_end - src_position_start);
    std::transform(
      dst_list.begin(),
      std::next(dst_list.begin(), dst_position),
      std::back_inserter(dst_expected_objects),
      to_pointer<T>);
    std::transform(
      std::next(src_list.begin(), src_position_start),
      std::next(src_list.begin(), src_position_end),
      std::back_inserter(dst_expected_objects),
      to_pointer<T>);
    std::transform(
      std::next(dst_list.begin(), dst_position),
      dst_list.end(),
      std::back_inserter(dst_expected_objects),
      to_pointer<T>);
    std::vector<T> dst_expected_values = get_vector_values(dst_expected_objects);

    dst_list.splice(
      std::next(dst_list.begin(), dst_position),
      src_list,
      std::next(src_list.begin(), src_position_start),
      std::next(src_list.begin(), src_position_end));

    SOUL_TEST_RUN(verify_sequence(src_list, src_expected_values, src_expected_objects));
    SOUL_TEST_RUN(verify_sequence(dst_list, dst_expected_values, dst_expected_objects));
  };

  SOUL_TEST_RUN(test_splice_list_range(
    random_list2.list,
    random_list1.list,
    0,
    random_list2.list.size(),
    random_list1.list.size() / 2));
  SOUL_TEST_RUN(test_splice_list_range(random_list3.list, random_list1.list, 0, 1, 0));
  SOUL_TEST_RUN(test_splice_list_range(
    random_list4.list,
    random_list1.list,
    0,
    random_list4.list.size() / 2,
    random_list1.list.size()));

  SOUL_TEST_RUN(test_splice_list_range(empty_list.list, random_list1.list, 0, 0, 0));
  SOUL_TEST_RUN(test_splice_list_range(single_list.list, random_list1.list, 0, 1, 0));
  SOUL_TEST_RUN(
    test_splice_list_range(random_list1.list, empty_list.list, 0, random_list1.list.size(), 0));
}

TEST(TestIntrusiveListClear, TestIntrusiveListClear)
{
  auto test_clear = []<typename T>(soul::IntrusiveList<T>& list)
  {
    list.clear();
    std::vector<T> empty_values;
    std::vector<T*> empty_objects;
    verify_sequence(list, empty_values, empty_objects);
  };
  RandomIntrusiveList<IntNode> random_list1(10);
  SOUL_TEST_RUN(test_clear(random_list1.list));

  RandomIntrusiveList<IntNode> empty_list(0);
  SOUL_TEST_RUN(test_clear(empty_list.list));
}
