#include <algorithm>
#include <concepts>
#include <xutility>
#include <gtest/gtest.h>

#include "core/config.h"
#include "core/cstring.h"
#include "core/log.h"
#include "core/meta.h"
#include "core/objops.h"
#include "core/panic.h"
#include "core/panic_lite.h"
#include "core/robin_table.h"
#include "core/type_traits.h"
#include "core/util.h"
#include "core/vector.h"
#include "core/views.h"
#include "memory/allocator.h"

#include "common_test.h"
#include "util.h"

namespace soul
{
  auto get_default_allocator() -> memory::Allocator*
  {
    static TestAllocator test_allocator("Test default allocator");
    return &test_allocator;
  }
} // namespace soul

struct TestEntry {
  soul::CString name;
  TestObject test_obj;
  using object_type = TestObject;

  [[nodiscard]]
  auto clone() const -> TestEntry
  {
    return {name.clone(), test_obj.clone()};
  }
  void clone_from(const TestEntry& test_entry)
  {
    name.clone_from(test_entry.name);
    test_obj.clone_from(test_entry.test_obj);
  }

  friend auto operator==(const TestEntry& rhs, const TestEntry& lhs) -> bool = default;

  struct GetKeyOp {
    auto operator()(const TestEntry& entry) const -> const soul::CString& { return entry.name; }
  };
};
static_assert(can_nontrivial_destruct_v<TestEntry>);

using TestTable = soul::RobinTable<soul::CString, TestEntry, TestEntry::GetKeyOp>;
static_assert(ts_clone<TestTable>);
static_assert(std::ranges::sized_range<TestTable>);

template <typename TableT>
auto verify_contain(const TableT& table, const typename TableT::value_type& entry)
{
  SOUL_TEST_ASSERT_TRUE(table.contains(entry.name));
  SOUL_TEST_ASSERT_EQ(table.entry_ref(entry.name), entry);
  SOUL_TEST_ASSERT_EQ(*table.find(entry.name), entry);
}

template <typename TableT>
auto verify_not_contain(const TableT& table, const typename TableT::key_type& key)
{
  SOUL_TEST_ASSERT_FALSE(table.contains(key));
  SOUL_TEST_ASSERT_EQ(table.find(key), table.end());
}

template <typename TableT>
auto verify_equal(const TableT& lhs, const TableT& rhs)
{
  SOUL_TEST_ASSERT_EQ(lhs.size(), rhs.size());
  const auto sort_fn = [](const TestEntry& a, const TestEntry& b) {
    return std::ranges::lexicographical_compare(a.name.cspan(), b.name.cspan());
  };

  for (const auto& entry : lhs) {
    verify_contain(rhs, entry);
  }

  for (const auto& entry : rhs) {
    verify_contain(lhs, entry);
  }
}

TEST(TestRobinTableConstruction, TestDefaultConstruction)
{
  TestTable test_table;
  SOUL_TEST_ASSERT_EQ(test_table.size(), 0);
  SOUL_TEST_ASSERT_EQ(test_table.begin(), test_table.end());
  SOUL_TEST_ASSERT_EQ(test_table.cbegin(), test_table.cend());
}

template <typename RobinTableT>
auto test_construction_with_capacity(usize capacity)
{
  const auto test_table = RobinTableT::WithCapacity(capacity);
  SOUL_TEST_ASSERT_EQ(test_table.size(), 0);
  SOUL_TEST_ASSERT_EQ(test_table.begin(), test_table.end());
  SOUL_TEST_ASSERT_EQ(test_table.cbegin(), test_table.cend());
  SOUL_TEST_ASSERT_GE(test_table.capacity(), capacity);
}

TEST(TestRobinTableConstruction, TestConstructionWithCapacity)
{
  SOUL_TEST_RUN(test_construction_with_capacity<TestTable>(0));
  SOUL_TEST_RUN(test_construction_with_capacity<TestTable>(100));
}

template <usize ArrSizeV>
auto test_construction_from_array(Array<TestEntry, ArrSizeV>&& entries)
{
  auto entry_vector = soul::Vector<TestEntry>::From(entries | soul::views::clone<TestEntry>());

  std::ranges::sort(entry_vector, [](const TestEntry& a, const TestEntry& b) {
    return std::ranges::lexicographical_compare(a.name.cspan(), b.name.cspan());
  });

  const auto test_table = TestTable::From(entries | soul::views::move<TestEntry>());

  SOUL_TEST_ASSERT_EQ(entry_vector.size(), test_table.size());
  for (const auto& entry : entry_vector) {
    verify_contain(test_table, entry);
  }
  soul::Vector<TestEntry> table_entries;
  for (const auto& entry : test_table) {
    table_entries.push_back(entry.clone());
  }
  std::ranges::sort(table_entries, [](const TestEntry& a, const TestEntry& b) {
    return std::ranges::lexicographical_compare(a.name.cspan(), b.name.cspan());
  });
  SOUL_TEST_ASSERT_EQ(entry_vector, table_entries);
}

TEST(TestRobinTableConstruction, TestConstructionFromRange)
{
  SOUL_TEST_RUN(test_construction_from_array(Array<TestEntry, 0>{}));

  SOUL_TEST_RUN(test_construction_from_array(Array{
    TestEntry{"kevin"_str, TestObject(3)},
  }));

  SOUL_TEST_RUN(test_construction_from_array(Array{
    TestEntry{"kevin"_str, TestObject(3)},
    TestEntry{"yudi"_str, TestObject(10)},
    TestEntry{"utama"_str, TestObject(1000)},
  }));

  SOUL_TEST_RUN(test_construction_from_array(Array{
    TestEntry{"kevin29"_str, TestObject(1000)}, TestEntry{"kevin27"_str, TestObject(1000)},
    TestEntry{"kevin26"_str, TestObject(1000)}, TestEntry{"kevin25"_str, TestObject(1000)},
    TestEntry{"kevin24"_str, TestObject(1000)}, TestEntry{"kevin23"_str, TestObject(1000)},
    TestEntry{"kevin22"_str, TestObject(1000)}, TestEntry{"kevin21"_str, TestObject(1000)},
    TestEntry{"kevin20"_str, TestObject(1000)}, TestEntry{"kevin19"_str, TestObject(1000)},
    TestEntry{"kevin18"_str, TestObject(1000)}, TestEntry{"kevin17"_str, TestObject(1000)},
    TestEntry{"kevin1"_str, TestObject(3)},     TestEntry{"kevin2"_str, TestObject(10)},
    TestEntry{"kevin3"_str, TestObject(1000)},  TestEntry{"kevin4"_str, TestObject(1000)},
    TestEntry{"kevin5"_str, TestObject(1000)},  TestEntry{"kevin6"_str, TestObject(1000)},
    TestEntry{"kevin7"_str, TestObject(1000)},  TestEntry{"kevin8"_str, TestObject(1000)},
    TestEntry{"kevin9"_str, TestObject(1000)},  TestEntry{"kevin10"_str, TestObject(1000)},
    TestEntry{"kevin11"_str, TestObject(1000)}, TestEntry{"kevin12"_str, TestObject(1000)},
    TestEntry{"kevin13"_str, TestObject(1000)}, TestEntry{"kevin14"_str, TestObject(1000)},
    TestEntry{"kevin15"_str, TestObject(1000)}, TestEntry{"kevin16"_str, TestObject(1000)},
  }));
}

TEST(TestRobinTableConstruction, TestMoveConstructor)
{
  SOUL_TEST_RUN(test_move_constructor(TestTable()));
  {
    auto test_table = TestTable::From(
      Array{
        TestEntry{"kevin29"_str, TestObject(1000)}, TestEntry{"kevin27"_str, TestObject(1000)},
        TestEntry{"kevin26"_str, TestObject(1000)}, TestEntry{"kevin25"_str, TestObject(1000)},
        TestEntry{"kevin24"_str, TestObject(1000)}, TestEntry{"kevin23"_str, TestObject(1000)},
        TestEntry{"kevin22"_str, TestObject(1000)}, TestEntry{"kevin21"_str, TestObject(1000)},
        TestEntry{"kevin20"_str, TestObject(1000)}, TestEntry{"kevin19"_str, TestObject(1000)},
        TestEntry{"kevin18"_str, TestObject(1000)}, TestEntry{"kevin17"_str, TestObject(1000)},
        TestEntry{"kevin1"_str, TestObject(3)},     TestEntry{"kevin2"_str, TestObject(10)},
        TestEntry{"kevin3"_str, TestObject(1000)},  TestEntry{"kevin4"_str, TestObject(1000)},
        TestEntry{"kevin5"_str, TestObject(1000)},  TestEntry{"kevin6"_str, TestObject(1000)},
        TestEntry{"kevin7"_str, TestObject(1000)},  TestEntry{"kevin8"_str, TestObject(1000)},
        TestEntry{"kevin9"_str, TestObject(1000)},  TestEntry{"kevin10"_str, TestObject(1000)},
        TestEntry{"kevin11"_str, TestObject(1000)}, TestEntry{"kevin12"_str, TestObject(1000)},
        TestEntry{"kevin13"_str, TestObject(1000)}, TestEntry{"kevin14"_str, TestObject(1000)},
        TestEntry{"kevin15"_str, TestObject(1000)}, TestEntry{"kevin16"_str, TestObject(1000)},
      } |
      soul::views::move<TestEntry>());
    SOUL_TEST_RUN(test_move_constructor(test_table));
  }
}

auto generate_random_entries(usize count) -> soul::Vector<TestEntry>
{

  std::random_device rd;
  std::mt19937 generator(rd());

  auto generate_random_string = [&generator](usize length, usize suffix_id) -> soul::CString {
    const char* char_samples =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()`~-_=+"
      "[{]{|;:'\",<.>/?";
    const auto char_samples_length = strlen(char_samples);

    auto output = CString::WithCapacity(length + util::digit_count(suffix_id));

    while (length > 0) {
      auto rand_numb = generator();
      while (rand_numb > char_samples_length && length > 0) {
        output.push_back(char_samples[rand_numb % char_samples_length]);
        rand_numb /= char_samples_length;
        length--;
      }
    }

    output.appendf("{}", suffix_id);

    return output;
  };

  return soul::Vector<TestEntry>::TransformIndex(
    0, count, [&generator, &generate_random_string](usize index) -> TestEntry {
      return {
        .name = generate_random_string(10, index),
        .test_obj = TestObject(static_cast<int>(generator())),
      };
    });
}

class TestRobinTableManipulation : public testing::Test
{
public:
  TestTable test_table1 =
    TestTable::From(generate_random_entries(1) | soul::views::move<TestEntry>());
  TestTable test_table2 =
    TestTable::From(generate_random_entries(1000) | soul::views::move<TestEntry>());
};

TEST_F(TestRobinTableManipulation, TestClone)
{
  SOUL_TEST_RUN(test_clone(TestTable()));
  SOUL_TEST_RUN(test_clone(test_table1));
  SOUL_TEST_RUN(test_clone(test_table2));
}

TEST_F(TestRobinTableManipulation, TestCloneFrom)
{
  SOUL_TEST_RUN(test_clone_from(test_table1, test_table2));
  SOUL_TEST_RUN(test_clone_from(test_table2, test_table1));
  SOUL_TEST_RUN(test_clone_from(TestTable(), test_table1));
  SOUL_TEST_RUN(test_clone_from(test_table1, TestTable()));
  SOUL_TEST_RUN(test_clone_from(TestTable(), test_table2));
  SOUL_TEST_RUN(test_clone_from(test_table2, TestTable()));
  SOUL_TEST_RUN(test_clone_from(TestTable(), test_table2));
  SOUL_TEST_RUN(test_clone_from(TestTable(), TestTable()));
}

TEST_F(TestRobinTableManipulation, TestMoveAssignment)
{
  SOUL_TEST_RUN(test_move_assignment(test_table1, test_table2));
  SOUL_TEST_RUN(test_move_assignment(test_table2, test_table1));
  SOUL_TEST_RUN(test_move_assignment(TestTable(), test_table1));
  SOUL_TEST_RUN(test_move_assignment(test_table1, TestTable()));
  SOUL_TEST_RUN(test_move_assignment(TestTable(), test_table2));
  SOUL_TEST_RUN(test_move_assignment(test_table2, TestTable()));
  SOUL_TEST_RUN(test_move_assignment(TestTable(), test_table2));
  SOUL_TEST_RUN(test_move_assignment(TestTable(), TestTable()));
}

TEST_F(TestRobinTableManipulation, TestSwap)
{
  SOUL_TEST_RUN(test_swap(test_table1, test_table2));
  SOUL_TEST_RUN(test_swap(test_table2, test_table1));
  SOUL_TEST_RUN(test_swap(TestTable(), test_table1));
  SOUL_TEST_RUN(test_swap(test_table1, TestTable()));
  SOUL_TEST_RUN(test_swap(TestTable(), test_table2));
  SOUL_TEST_RUN(test_swap(test_table2, TestTable()));
  SOUL_TEST_RUN(test_swap(TestTable(), test_table2));
  SOUL_TEST_RUN(test_swap(TestTable(), TestTable()));
}

TEST_F(TestRobinTableManipulation, TestClear)
{
  const auto test_clear = []<typename TableT>(const TableT& table) {
    auto test_table = table.clone();
    test_table.clear();

    SOUL_TEST_ASSERT_EQ(test_table.size(), 0);
    for (const auto& entry : table) {
      SOUL_TEST_ASSERT_FALSE(test_table.contains(entry.name));
      SOUL_TEST_ASSERT_EQ(test_table.find(entry.name), test_table.end());
    }
  };

  SOUL_TEST_RUN(test_clear(TestTable()));
  SOUL_TEST_RUN(test_clear(test_table1));
  SOUL_TEST_RUN(test_clear(test_table2));
}

TEST_F(TestRobinTableManipulation, TestCleanup)
{
  const auto test_cleanup = []<typename TableT>(const TableT& table) {
    auto test_table = table.clone();
    test_table.cleanup();

    SOUL_TEST_ASSERT_EQ(test_table.size(), 0);
    for (const auto& entry : table) {
      SOUL_TEST_ASSERT_FALSE(test_table.contains(entry.name));
      SOUL_TEST_ASSERT_EQ(test_table.find(entry.name), test_table.end());
    }

    SOUL_TEST_ASSERT_EQ(test_table.capacity(), 0);
  };

  SOUL_TEST_RUN(test_cleanup(TestTable()));
  SOUL_TEST_RUN(test_cleanup(test_table1));
  SOUL_TEST_RUN(test_cleanup(test_table2));
}

TEST_F(TestRobinTableManipulation, TestReserve)
{
  SOUL_TEST_RUN(test_reserve(TestTable(), 10));
  SOUL_TEST_RUN(test_reserve(test_table1, 0));
  SOUL_TEST_RUN(test_reserve(test_table1, 10));
  SOUL_TEST_RUN(test_reserve(test_table2, 0));
  SOUL_TEST_RUN(test_reserve(test_table2, 1));
  SOUL_TEST_RUN(test_reserve(test_table2, test_table2.size() / 2));
  SOUL_TEST_RUN(test_reserve(test_table2, test_table2.size() * 2));
}

TEST_F(TestRobinTableManipulation, TestInsert)
{
  const auto test_insert = []<typename TableT>(const TableT& table) {
    auto test_table = table.clone();
    const auto initial_size = table.size();

    const auto test_entry1 = TestEntry{"soul_test_str"_str, TestObject(3)};
    test_table.insert(test_entry1.clone());
    verify_contain(test_table, test_entry1);

    const auto test_entry2 = TestEntry{"soul_test_str"_str, TestObject(5)};
    test_table.insert(test_entry2.clone());
    verify_contain(test_table, test_entry2);

    const auto RANDOM_INSERT_COUNT = 1000;
    const auto random_test_entries = generate_random_entries(RANDOM_INSERT_COUNT);
    for (const auto& random_entry : random_test_entries) {
      test_table.insert(random_entry.clone());
    }
    for (const auto& random_entry : random_test_entries) {
      verify_contain(test_table, random_entry);
    }

    SOUL_TEST_ASSERT_EQ(test_table.size(), initial_size + 1 + RANDOM_INSERT_COUNT);
  };

  SOUL_TEST_RUN(test_insert(TestTable()));
  SOUL_TEST_RUN(test_insert(test_table1));
  SOUL_TEST_RUN(test_insert(test_table2));
}

TEST_F(TestRobinTableManipulation, TestRemove)
{
  {
    auto test_table = TestTable();
    test_table.remove("soul_test_str"_str);
    SOUL_TEST_RUN(verify_not_contain(test_table, "soul_test_str"_str));
    SOUL_TEST_ASSERT_EQ(test_table.size(), 0);
  }

  {
    const auto name = test_table1.begin()->name.clone();
    test_table1.remove(name);
    SOUL_TEST_RUN(verify_not_contain(test_table1, name));
    SOUL_TEST_ASSERT_EQ(test_table1.size(), 0);
  }

  {
    const auto initial_size = test_table2.size();
    auto middle_iter = test_table2.begin();
    for (auto i = 0; i < initial_size / 2; i++) {
      middle_iter++;
    }
    const auto names = Array{
      test_table2.begin()->name.clone(),
      middle_iter->name.clone(),
    };

    for (const auto& name : names) {
      test_table2.remove(name);
    }
    for (const auto& name : names) {
      SOUL_TEST_RUN(verify_not_contain(test_table2, name));
    }
  }
}
