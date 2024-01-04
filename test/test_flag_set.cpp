#include <algorithm>
#include <random>
#include <ranges>
#include <vector>

#include "core/flag_set.h"
#include "core/util.h"

#include "large_uint64_enum.h"
#include "util.h"

enum class Uint8TestEnum : u8
{
  ONE,
  TWO,
  THREE,
  FOUR,
  FIVE,
  SIX,
  COUNT
};

enum class Uint16TestEnum : u16
{
  ONE,
  TWO,
  THREE,
  FOUR,
  FIVE,
  SIX,
  COUNT
};

enum class Uint32TestEnum : u32
{
  ONE,
  TWO,
  THREE,
  FOUR,
  FIVE,
  SIX,
  COUNT
};

enum class Uint64TestEnum : u64
{
  ONE,
  TWO,
  THREE,
  FOUR,
  FIVE,
  SIX,
  COUNT
};

using Uint8FlagSet  = soul::FlagSet<Uint8TestEnum>;
using Uint16FlagSet = soul::FlagSet<Uint16TestEnum>;
using Uint32FlagSet = soul::FlagSet<Uint32TestEnum>;
using Uint64FlagSet = soul::FlagSet<Uint64TestEnum>;

template <soul::ts_scoped_enum T>
auto test_default_constructor() -> void
{
  constexpr soul::FlagSet<T> flag_set;
  SOUL_TEST_ASSERT_EQ(flag_set.size(), soul::to_underlying(T::COUNT));
  SOUL_TEST_ASSERT_TRUE(flag_set.none());
  SOUL_TEST_ASSERT_FALSE(flag_set.any());
  for (const T e : soul::FlagIter<T>())
  {
    SOUL_TEST_ASSERT_FALSE(flag_set.test(e));
    SOUL_TEST_ASSERT_FALSE(flag_set[e]);
  }
};

TEST(TestFlagSetConstructor, TestFlagSetDefaultConstructor)
{
  SOUL_TEST_RUN(test_default_constructor<Uint8TestEnum>());
  SOUL_TEST_RUN(test_default_constructor<Uint16TestEnum>());
  SOUL_TEST_RUN(test_default_constructor<Uint32TestEnum>());
  SOUL_TEST_RUN(test_default_constructor<Uint64TestEnum>());
}

template <soul::ts_scoped_enum T>
auto test_init_list_constructor(std::initializer_list<T> init_list) -> void
{
  const soul::FlagSet<T> flag_set = init_list;
  SOUL_TEST_ASSERT_EQ(flag_set.size(), soul::to_underlying(T::COUNT));
  SOUL_TEST_ASSERT_FALSE(flag_set.none());
  SOUL_TEST_ASSERT_TRUE(flag_set.any());
  for (const T e : soul::FlagIter<T>())
  {
    if (std::find(init_list.begin(), init_list.end(), e) == init_list.end())
    {
      SOUL_TEST_ASSERT_FALSE(flag_set.test(e));
      SOUL_TEST_ASSERT_FALSE(flag_set[e]);
    } else
    {
      SOUL_TEST_ASSERT_TRUE(flag_set.test(e));
      SOUL_TEST_ASSERT_TRUE(flag_set[e]);
    }
  }
}

TEST(TestFlagSetConstructor, TestFlagSetInitListConstructor)
{
  SOUL_TEST_RUN(
    test_init_list_constructor({Uint8TestEnum::ONE, Uint8TestEnum::THREE, Uint8TestEnum::SIX}));
  SOUL_TEST_RUN(test_init_list_constructor({Uint32TestEnum::SIX}));
}

TEST(TestFlagSetConstructor, TestFlagSetCopyConstructor)
{
  const Uint8FlagSet test_filled_flag_set      = {Uint8TestEnum::ONE, Uint8TestEnum::SIX};
  const Uint8FlagSet test_copy_filled_flag_set = test_filled_flag_set;
  SOUL_TEST_ASSERT_EQ(test_filled_flag_set, test_copy_filled_flag_set);
  SOUL_TEST_ASSERT_EQ(test_filled_flag_set.count(), test_copy_filled_flag_set.count());
  SOUL_TEST_ASSERT_EQ(test_filled_flag_set.size(), test_copy_filled_flag_set.size());

  const Uint8FlagSet test_empty_flag_set;
  const Uint8FlagSet test_copy_empty_flag_set = test_empty_flag_set;
  SOUL_TEST_ASSERT_EQ(test_copy_empty_flag_set, test_empty_flag_set);
  SOUL_TEST_ASSERT_EQ(test_copy_empty_flag_set.count(), 0);
  SOUL_TEST_ASSERT_EQ(test_copy_empty_flag_set.size(), test_empty_flag_set.size());
}

TEST(TestFlagSetConstructor, TestFlagSetMoveConstructor)
{
  Uint8FlagSet test_filled_flag_set            = {Uint8TestEnum::ONE, Uint8TestEnum::SIX};
  const Uint8FlagSet test_copy_filled_flag_set = test_filled_flag_set;
  const Uint8FlagSet test_move_filled_flag_set =
    std::move(test_filled_flag_set); // NOLINT(performance-move-const-arg)
  SOUL_TEST_ASSERT_EQ(test_move_filled_flag_set, test_copy_filled_flag_set);

  Uint8FlagSet test_empty_flag_set;
  const Uint8FlagSet test_move_empty_flag_set =
    std::move(test_empty_flag_set); // NOLINT(performance-move-const-arg)
  SOUL_TEST_ASSERT_EQ(test_move_empty_flag_set, Uint8FlagSet());
  SOUL_TEST_ASSERT_EQ(test_move_empty_flag_set.count(), 0);
}

TEST(TestFlagSetAssignment, TestFlagSetAssignment)
{
  Uint8FlagSet test_filled_flag_set = {Uint8TestEnum::ONE, Uint8TestEnum::SIX};
  // ReSharper disable once CppJoinDeclarationAndAssignment
  Uint8FlagSet test_copy_filled_flag_set;
  test_copy_filled_flag_set = test_filled_flag_set;
  SOUL_TEST_ASSERT_EQ(test_filled_flag_set, test_copy_filled_flag_set);
  SOUL_TEST_ASSERT_EQ(test_filled_flag_set.count(), test_copy_filled_flag_set.count());
  SOUL_TEST_ASSERT_EQ(test_filled_flag_set.size(), test_copy_filled_flag_set.size());
  // ReSharper disable once CppJoinDeclarationAndAssignment
  Uint8FlagSet test_move_filled_flag_set;
  test_move_filled_flag_set = std::move(test_filled_flag_set); // NOLINT(performance-move-const-arg)
  SOUL_TEST_ASSERT_EQ(test_move_filled_flag_set, test_copy_filled_flag_set);

  Uint8FlagSet test_empty_flag_set;
  // ReSharper disable once CppInitializedValueIsAlwaysRewritten
  Uint8FlagSet test_copy_empty_flag_set = {Uint8TestEnum::ONE};
  test_copy_empty_flag_set              = test_empty_flag_set;
  SOUL_TEST_ASSERT_EQ(test_copy_empty_flag_set, test_empty_flag_set);
  SOUL_TEST_ASSERT_EQ(test_copy_empty_flag_set.count(), 0);
  // ReSharper disable once CppInitializedValueIsAlwaysRewritten
  Uint8FlagSet test_move_empty_flag_set = {
    Uint8TestEnum::SIX}; // NOLINT(performance-move-const-arg)
  test_move_empty_flag_set = std::move(test_empty_flag_set);
  SOUL_TEST_ASSERT_EQ(test_move_empty_flag_set, Uint8FlagSet());
  SOUL_TEST_ASSERT_EQ(test_move_empty_flag_set.count(), 0);
}

class TestFlagSetManipulation : public testing::Test
{
public:
  Uint8FlagSet test_filled_flag_set = {Uint8TestEnum::TWO, Uint8TestEnum::FOUR};
  Uint16FlagSet test_empty_flag_set;
};

TEST_F(TestFlagSetManipulation, TestFlagSetSet)
{
  auto test_set = []<soul::ts_scoped_enum T>(soul::FlagSet<T> test_flag_set)
  {
    test_flag_set.set();
    SOUL_TEST_ASSERT_EQ(test_flag_set.count(), soul::to_underlying(T::COUNT));
    SOUL_TEST_ASSERT_FALSE(test_flag_set.none());
    SOUL_TEST_ASSERT_TRUE(test_flag_set.any());
    for (auto e : soul::FlagIter<T>())
    {
      SOUL_TEST_ASSERT_TRUE(test_flag_set.test(e));
      SOUL_TEST_ASSERT_TRUE(test_flag_set[e]);
    }
  };

  SOUL_TEST_RUN(test_set(test_filled_flag_set));
  SOUL_TEST_RUN(test_set(test_empty_flag_set));

  auto test_set_pos =
    []<soul::ts_scoped_enum T>(soul::FlagSet<T> test_flag_set, T position, b8 value)
  {
    auto old_flag_set = test_flag_set;
    test_flag_set.set(position, value);
    SOUL_TEST_ASSERT_EQ(test_flag_set.test(position), value);
    SOUL_TEST_ASSERT_EQ(test_flag_set[position], value);
    for (auto e : soul::FlagIter<T>())
    {
      if (e != position)
      {
        SOUL_TEST_ASSERT_EQ(test_flag_set.test(e), old_flag_set.test(e));
        SOUL_TEST_ASSERT_EQ(test_flag_set[e], old_flag_set[e]);
      }
    }
    if (value)
    {
      if (old_flag_set.test(position))
      {
        SOUL_TEST_ASSERT_EQ(test_flag_set.count(), old_flag_set.count());
      } else
      {
        SOUL_TEST_ASSERT_EQ(test_flag_set.count(), old_flag_set.count() + 1);
      }
    } else
    {
      if (old_flag_set.test(position))
      {
        SOUL_TEST_ASSERT_EQ(test_flag_set.count(), old_flag_set.count() - 1);
      } else
      {
        SOUL_TEST_ASSERT_EQ(test_flag_set.count(), old_flag_set.count());
      }
    }
  };

  SOUL_TEST_RUN(test_set_pos(test_filled_flag_set, Uint8TestEnum::THREE, true));
  SOUL_TEST_RUN(test_set_pos(test_filled_flag_set, Uint8TestEnum::THREE, false));
  SOUL_TEST_RUN(test_set_pos(test_filled_flag_set, Uint8TestEnum::TWO, true));
  SOUL_TEST_RUN(test_set_pos(test_filled_flag_set, Uint8TestEnum::TWO, false));

  SOUL_TEST_RUN(test_set_pos(test_empty_flag_set, Uint16TestEnum::THREE, true));
  SOUL_TEST_RUN(test_set_pos(test_empty_flag_set, Uint16TestEnum::SIX, true));
  SOUL_TEST_RUN(test_set_pos(test_empty_flag_set, Uint16TestEnum::ONE, true));
  SOUL_TEST_RUN(test_set_pos(test_empty_flag_set, Uint16TestEnum::THREE, false));
  SOUL_TEST_RUN(test_set_pos(test_empty_flag_set, Uint16TestEnum::SIX, false));
  SOUL_TEST_RUN(test_set_pos(test_empty_flag_set, Uint16TestEnum::ONE, false));
}

TEST_F(TestFlagSetManipulation, TestFlagSetReset)
{
  auto test_reset = []<soul::ts_scoped_enum T>(soul::FlagSet<T> test_flag_set)
  {
    test_flag_set.reset();
    SOUL_TEST_ASSERT_EQ(test_flag_set.count(), 0);
    SOUL_TEST_ASSERT_FALSE(test_flag_set.any());
    SOUL_TEST_ASSERT_TRUE(test_flag_set.none());
    for (auto e : soul::FlagIter<T>())
    {
      SOUL_TEST_ASSERT_FALSE(test_flag_set.test(e));
      SOUL_TEST_ASSERT_FALSE(test_flag_set[e]);
    }
  };

  SOUL_TEST_RUN(test_reset(test_filled_flag_set));
  SOUL_TEST_RUN(test_reset(test_empty_flag_set));

  auto test_reset_position = []<soul::ts_scoped_enum T>(soul::FlagSet<T> test_flag_set, T position)
  {
    auto old_flag_set = test_flag_set;
    test_flag_set.reset(position);
    SOUL_TEST_ASSERT_FALSE(test_flag_set.test(position));
    SOUL_TEST_ASSERT_FALSE(test_flag_set[position]);
    for (auto e : soul::FlagIter<T>())
    {
      if (e != position)
      {
        SOUL_TEST_ASSERT_EQ(test_flag_set.test(e), old_flag_set.test(e));
      }
    }

    if (old_flag_set.test(position))
    {
      SOUL_TEST_ASSERT_EQ(test_flag_set.count(), old_flag_set.count() - 1);
    } else
    {
      SOUL_TEST_ASSERT_EQ(test_flag_set.count(), old_flag_set.count());
    }
  };

  SOUL_TEST_RUN(test_reset_position(test_filled_flag_set, Uint8TestEnum::TWO));
  SOUL_TEST_RUN(test_reset_position(test_filled_flag_set, Uint8TestEnum::ONE));

  SOUL_TEST_RUN(test_reset_position(test_empty_flag_set, Uint16TestEnum::TWO));
  SOUL_TEST_RUN(test_reset_position(test_empty_flag_set, Uint16TestEnum::ONE));
}

TEST_F(TestFlagSetManipulation, TestFlagSetFlip)
{
  auto test_flip = []<soul::ts_scoped_enum T>(soul::FlagSet<T> test_flag_set)
  {
    auto old_flag_set = test_flag_set;
    test_flag_set.flip();
    for (auto e : soul::FlagIter<T>())
    {
      SOUL_TEST_ASSERT_EQ(test_flag_set.test(e), !old_flag_set.test(e));
      SOUL_TEST_ASSERT_EQ(test_flag_set[e], !old_flag_set[e]);
    }
    SOUL_TEST_ASSERT_EQ(test_flag_set.count(), old_flag_set.size() - old_flag_set.count());
  };

  SOUL_TEST_RUN(test_flip(test_filled_flag_set));
  SOUL_TEST_RUN(test_flip(test_empty_flag_set));

  auto test_flip_position = []<soul::ts_scoped_enum T>(soul::FlagSet<T> test_flag_set, T position)
  {
    auto old_flag_set = test_flag_set;
    test_flag_set.flip(position);
    for (auto e : soul::FlagIter<T>())
    {
      if (e != position)
      {
        SOUL_TEST_ASSERT_EQ(test_flag_set.test(e), old_flag_set.test(e));
      } else
      {
        SOUL_TEST_ASSERT_EQ(test_flag_set.test(e), !old_flag_set.test(e));
      }
    }
    if (old_flag_set.test(position))
    {
      SOUL_TEST_ASSERT_EQ(test_flag_set.count(), old_flag_set.count() - 1);
    } else
    {
      SOUL_TEST_ASSERT_EQ(test_flag_set.count(), old_flag_set.count() + 1);
    }
  };

  SOUL_TEST_RUN(test_flip_position(test_filled_flag_set, Uint8TestEnum::TWO));
  SOUL_TEST_RUN(test_flip_position(test_filled_flag_set, Uint8TestEnum::ONE));

  SOUL_TEST_RUN(test_flip_position(test_empty_flag_set, Uint16TestEnum::THREE));
}

TEST(TestFlagSetOperator, TestFlagSetOperatorOr)
{
  auto test_operator_or = []<typename T>(soul::FlagSet<T> flag_set1, soul::FlagSet<T> flag_set2)
  {
    auto flag_set_result = flag_set1 | flag_set2;
    usize expected_count = 0;
    for (auto e : soul::FlagIter<T>())
    {
      if (flag_set1.test(e) || flag_set2.test(e))
      {
        SOUL_TEST_ASSERT_TRUE(flag_set_result.test(e));
        expected_count++;
      } else
      {
        SOUL_TEST_ASSERT_FALSE(flag_set_result.test(e));
      }
    }
    SOUL_TEST_ASSERT_EQ(flag_set_result.count(), expected_count);
    flag_set1 |= flag_set2;
    SOUL_TEST_ASSERT_EQ(flag_set_result, flag_set1);
    SOUL_TEST_ASSERT_EQ(flag_set1.count(), expected_count);
  };

  SOUL_TEST_RUN(test_operator_or(
    Uint16FlagSet({Uint16TestEnum::ONE, Uint16TestEnum::TWO}),
    Uint16FlagSet({Uint16TestEnum::TWO, Uint16TestEnum::THREE})));
  SOUL_TEST_RUN(test_operator_or(Uint8FlagSet(), Uint8FlagSet({Uint8TestEnum::THREE})));
  SOUL_TEST_RUN(test_operator_or(Uint8FlagSet(), Uint8FlagSet()));
}

TEST(TestFlagSetOperator, TestFlagSetOperatorAnd)
{
  auto test_operator_and = []<typename T>(soul::FlagSet<T> flag_set1, soul::FlagSet<T> flag_set2)
  {
    auto flag_set_result = flag_set1 & flag_set2;
    usize expected_count = 0;
    for (auto e : soul::FlagIter<T>())
    {
      if (flag_set1.test(e) && flag_set2.test(e))
      {
        expected_count++;
        SOUL_TEST_ASSERT_TRUE(flag_set_result.test(e));
      } else
      {
        SOUL_TEST_ASSERT_FALSE(flag_set_result.test(e));
      }
    }
    SOUL_TEST_ASSERT_EQ(flag_set_result.count(), expected_count);
    flag_set1 &= flag_set2;
    SOUL_TEST_ASSERT_EQ(flag_set_result, flag_set1);
    SOUL_TEST_ASSERT_EQ(flag_set1.count(), expected_count);
  };

  SOUL_TEST_RUN(test_operator_and(
    Uint16FlagSet({Uint16TestEnum::ONE, Uint16TestEnum::TWO}), {Uint16TestEnum::FOUR}));
  SOUL_TEST_RUN(test_operator_and(
    Uint8FlagSet({Uint8TestEnum::ONE, Uint8TestEnum::TWO}), {Uint8TestEnum::TWO}));
  SOUL_TEST_RUN(test_operator_and(Uint8FlagSet(), {Uint8TestEnum::TWO, Uint8TestEnum::FOUR}));
  SOUL_TEST_RUN(
    test_operator_and(Uint8FlagSet({Uint8TestEnum::ONE, Uint8TestEnum::TWO}), Uint8FlagSet()));
  SOUL_TEST_RUN(test_operator_and(Uint8FlagSet(), Uint8FlagSet()));
}

TEST(TestFlagSetOperator, TestFlagSetOperatorXor)
{
  auto test_operator_xor = []<typename T>(soul::FlagSet<T> flag_set1, soul::FlagSet<T> flag_set2)
  {
    auto flag_set_result = flag_set1 ^ flag_set2;
    usize expected_count = 0;
    for (auto e : soul::FlagIter<T>())
    {
      if (flag_set1.test(e) != flag_set2.test(e))
      {
        expected_count++;
        SOUL_TEST_ASSERT_TRUE(flag_set_result.test(e));
      } else
      {
        SOUL_TEST_ASSERT_FALSE(flag_set_result.test(e));
      }
    }
    SOUL_TEST_ASSERT_EQ(flag_set_result.count(), expected_count);
    flag_set1 ^= flag_set2;
    SOUL_TEST_ASSERT_EQ(flag_set_result, flag_set1);
    SOUL_TEST_ASSERT_EQ(flag_set1.count(), expected_count);
  };

  SOUL_TEST_RUN(test_operator_xor(
    Uint16FlagSet({Uint16TestEnum::ONE, Uint16TestEnum::TWO}), {Uint16TestEnum::FOUR}));
  SOUL_TEST_RUN(test_operator_xor(
    Uint8FlagSet({Uint8TestEnum::ONE, Uint8TestEnum::TWO}), {Uint8TestEnum::TWO}));
  SOUL_TEST_RUN(test_operator_xor(Uint8FlagSet(), {Uint8TestEnum::TWO, Uint8TestEnum::FOUR}));
  SOUL_TEST_RUN(
    test_operator_xor(Uint8FlagSet({Uint8TestEnum::ONE, Uint8TestEnum::TWO}), Uint8FlagSet()));
  SOUL_TEST_RUN(test_operator_xor(Uint8FlagSet(), Uint8FlagSet()));
}

TEST(TestFlagSetOperator, TestFlagSetOperatorNegate)
{
  auto test_operator_negate = []<typename T>(soul::FlagSet<T> flag_set)
  {
    auto flag_set_result = ~flag_set;
    for (auto e : soul::FlagIter<T>())
    {
      if (flag_set.test(e))
      {
        SOUL_TEST_ASSERT_FALSE(flag_set_result.test(e));
      } else
      {
        SOUL_TEST_ASSERT_TRUE(flag_set_result.test(e));
      }
    }
    SOUL_TEST_ASSERT_EQ(flag_set_result.count(), flag_set.size() - flag_set.count());
  };
  SOUL_TEST_RUN(test_operator_negate(Uint8FlagSet({Uint8TestEnum::ONE, Uint8TestEnum::TWO})));
  SOUL_TEST_RUN(test_operator_negate(Uint8FlagSet()));
}

TEST(TestFlagSetMap, TestFlagSetMap)
{
  const Uint8FlagSet test_filled_flag_set = {Uint8TestEnum::ONE, Uint8TestEnum::THREE};
  const auto filled_map_result            = test_filled_flag_set.map({1, 2, 3, 4, 5, 6});
  SOUL_TEST_ASSERT_EQ(filled_map_result, 1 | 3);

  const auto filled_map_result2 = test_filled_flag_set.map<Uint16FlagSet>(
    {{Uint16TestEnum::ONE},
     {Uint16TestEnum::TWO},
     {Uint16TestEnum::THREE},
     {Uint16TestEnum::FOUR},
     {Uint16TestEnum::FIVE},
     {Uint16TestEnum::SIX}});
  const auto expected_filled_map_result2 =
    Uint16FlagSet{Uint16TestEnum::ONE, Uint16TestEnum::THREE};
  SOUL_TEST_ASSERT_EQ(filled_map_result2, expected_filled_map_result2);

  const Uint8FlagSet test_empty_flag_set;
  const auto empty_map_result = test_empty_flag_set.map({1, 2, 3, 4, 5, 6});
  SOUL_TEST_ASSERT_EQ(empty_map_result, 0);
  const auto empty_map_result2 = test_empty_flag_set.map<Uint16FlagSet>(
    {{Uint16TestEnum::ONE},
     {Uint16TestEnum::TWO},
     {Uint16TestEnum::THREE},
     {Uint16TestEnum::FOUR},
     {Uint16TestEnum::FIVE},
     {Uint16TestEnum::SIX}});
  SOUL_TEST_ASSERT_EQ(empty_map_result2, Uint16FlagSet{});
}

TEST(TestFlagSetForEach, TestFlagSetForEach)
{
  std::vector<Uint8TestEnum> filled_vector_result;
  const Uint8FlagSet test_filled_flag_set = {Uint8TestEnum::ONE, Uint8TestEnum::THREE};
  test_filled_flag_set.for_each(
    [&filled_vector_result](const Uint8TestEnum val)
    {
      filled_vector_result.push_back(val);
    });
  SOUL_TEST_ASSERT_EQ(filled_vector_result.size(), 2);
  SOUL_TEST_ASSERT_EQ(filled_vector_result[0], Uint8TestEnum::ONE);
  SOUL_TEST_ASSERT_EQ(filled_vector_result[1], Uint8TestEnum::THREE);

  std::vector<Uint8TestEnum> empty_vector_result;
  const Uint8FlagSet test_empty_flag_set;
  test_empty_flag_set.for_each(
    [&empty_vector_result](const Uint8TestEnum val)
    {
      empty_vector_result.push_back(val);
    });
  SOUL_TEST_ASSERT_EQ(empty_vector_result.size(), 0);
}

TEST(TestFlagSetFindIf, TestFlagSetFindIf)
{
  std::vector<Uint8TestEnum> filled_vector_result;
  const Uint8FlagSet test_filled_flag_set = {Uint8TestEnum::ONE, Uint8TestEnum::THREE};
  SOUL_TEST_ASSERT_EQ(
    test_filled_flag_set
      .find_if(
        [val_to_find = Uint8TestEnum::THREE](const Uint8TestEnum val)
        {
          return val == val_to_find;
        })
      .unwrap(),
    Uint8TestEnum::THREE);
  SOUL_TEST_ASSERT_FALSE(test_filled_flag_set
                           .find_if(
                             [val_to_find = Uint8TestEnum::TWO](const Uint8TestEnum val)
                             {
                               return val == val_to_find;
                             })
                           .is_some());

  const Uint8FlagSet test_empty_flag_set;
  SOUL_TEST_ASSERT_FALSE(test_empty_flag_set
                           .find_if(
                             [val_to_find = Uint8TestEnum::THREE](const Uint8TestEnum val)
                             {
                               return val == val_to_find;
                             })
                           .is_some());
}

TEST(TestFlagSetToUint, TestFlagSetToUint)
{
  SOUL_TEST_ASSERT_EQ(Uint8FlagSet({}).to_uint32(), 0);
  SOUL_TEST_ASSERT_EQ(Uint8FlagSet({Uint8TestEnum::ONE, Uint8TestEnum::THREE}).to_uint32(), 5);
  SOUL_TEST_ASSERT_EQ(Uint32FlagSet({Uint32TestEnum::ONE, Uint32TestEnum::THREE}).to_uint32(), 5);
  SOUL_TEST_ASSERT_EQ(Uint64FlagSet({Uint64TestEnum::ONE, Uint64TestEnum::THREE}).to_uint32(), 5);

  SOUL_TEST_ASSERT_EQ(Uint8FlagSet({}).to_uint64(), 0u);
  SOUL_TEST_ASSERT_EQ(Uint8FlagSet({Uint8TestEnum::ONE, Uint8TestEnum::THREE}).to_uint64(), 5u);
  SOUL_TEST_ASSERT_EQ(Uint32FlagSet({Uint32TestEnum::ONE, Uint32TestEnum::THREE}).to_uint64(), 5u);
  SOUL_TEST_ASSERT_EQ(Uint64FlagSet({Uint64TestEnum::ONE, Uint64TestEnum::THREE}).to_uint64(), 5u);

  // test fail compilation. Uncomment tests below, expected to generate compilation error since
  // the LargeUint64TestEnum::COUNT exceeded the width of the u32
  // SOUL_TEST_ASSERT_EQ(LargeUint64FlagSet({ LargeUint64TestEnum::ONE, LargeUint64TestEnum::THREE
  // }).to_uint32(), 5u);
}
