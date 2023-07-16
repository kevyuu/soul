#include <gtest/gtest.h>

#include "core/config.h"
#include "core/objops.h"
#include "core/option.h"
#include "core/type_traits.h"
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

using OptInt = soul::Option<int>;
using OptObj = soul::Option<TestObject>;
using OptListObj = soul::Option<ListTestObject>;

template <typename T>
auto test_default_constructor() -> void
{
  soul::Option<T> option;
  SOUL_TEST_ASSERT_FALSE(option.is_some());
}

template <typename T>
auto verify_option_equal(const soul::Option<T>& opt1, const soul::Option<T>& opt2)
{
  SOUL_TEST_ASSERT_EQ(opt1.is_some(), opt2.is_some());
  if (opt1.is_some() && opt2.is_some()) {
    SOUL_TEST_ASSERT_EQ(opt1.some_ref(), opt2.some_ref());
  }
  SOUL_TEST_ASSERT_EQ(opt1, opt2);
}

TEST(TestOptionConstruction, TestDefaultConstructor)
{
  SOUL_TEST_RUN(test_default_constructor<int>());
  SOUL_TEST_RUN(test_default_constructor<TestObject>());
  SOUL_TEST_RUN(test_default_constructor<ListTestObject>());
}

template <soul::typeset T>
auto test_construction_some(const T& val)
{
  auto option = soul::Option<T>::some(soul::duplicate(val));
  SOUL_TEST_ASSERT_TRUE(option.is_some());
  SOUL_TEST_ASSERT_EQ(option.some_ref(), val);
  if constexpr (soul::can_trivial_copy_v<T>) {
    SOUL_TEST_ASSERT_EQ(option.unwrap(), val);
  }
  SOUL_TEST_ASSERT_EQ(std::move(option).unwrap(), val);
}

TEST(TestOptionConstruction, TestConstructionSome)
{
  SOUL_TEST_RUN(test_construction_some<int>(5));
  SOUL_TEST_RUN(test_construction_some<TestObject>(TestObject(5)));
  SOUL_TEST_RUN(test_construction_some<ListTestObject>(
    ListTestObject::generate_n([] { return TestObject(10); }, 5)));
}

template <soul::typeset T>
auto test_copy_constructor(const soul::Option<T>& opt_src)
{
  const auto opt_dst = opt_src;
  verify_option_equal(opt_dst, opt_src);
}

TEST(TestOptionConstruction, TestCopyConstructor)
{
  SOUL_TEST_RUN(test_copy_constructor<int>(OptInt()));
  SOUL_TEST_RUN(test_copy_constructor<int>(OptInt::some(3)));
}

template <soul::typeset T>
auto test_clone(const soul::Option<T>& opt_src)
{
  const auto opt_dst = opt_src.clone();
  verify_option_equal<T>(opt_dst, opt_src);
}

auto test_object_factory = [] { return TestObject(3); };

TEST(TestOptionConstruction, TestClone)
{
  SOUL_TEST_RUN(test_clone(OptObj()));
  SOUL_TEST_RUN(test_clone(OptObj::some(TestObject(5))));

  SOUL_TEST_RUN(test_clone(OptListObj()));
  SOUL_TEST_RUN(test_clone(OptListObj::some(ListTestObject::generate_n(test_object_factory, 10))));
}

template <soul::typeset T>
auto test_move_constructor(const soul::Option<T>& opt_src)
{
  auto opt_src_clone = opt_src.clone();
  const auto opt_dst = std::move(opt_src_clone);
  verify_option_equal(opt_dst, opt_src);
}

TEST(TestOptionConstruction, TestMoveConstructor)
{
  SOUL_TEST_RUN(test_move_constructor(OptObj()));
  SOUL_TEST_RUN(test_move_constructor(OptObj::some(TestObject(5))));

  SOUL_TEST_RUN(test_move_constructor(OptListObj()));
  SOUL_TEST_RUN(
    test_move_constructor(OptListObj::some(ListTestObject::generate_n(test_object_factory, 10))));
}

template <typename T>
auto test_copy_assignment(const soul::Option<T>& opt_src, const soul::Option<T>& sample_opt_dst)
{
  auto opt_dst = sample_opt_dst;
  opt_dst = opt_src;
  verify_option_equal<T>(opt_dst, opt_src);
}

TEST(TestOptionAssignment, TestCopyAssignment)
{
  SOUL_TEST_RUN(test_copy_assignment<int>(OptInt(), OptInt()));
  SOUL_TEST_RUN(test_copy_assignment<int>(OptInt(), OptInt::some(3)));
  SOUL_TEST_RUN(test_copy_assignment<int>(OptInt::some(3), OptInt()));
  SOUL_TEST_RUN(test_copy_assignment<int>(OptInt::some(3), OptInt::some(3)));
}

template <typename T>
auto test_clone_from(const soul::Option<T>& opt_src, const soul::Option<T>& sample_opt_dst)
{
  auto opt_dst = sample_opt_dst.clone();
  opt_dst.clone_from(opt_src);
  verify_option_equal<T>(opt_dst, opt_src);
}

TEST(TestOptionConstruction, TestCloneFrom)
{
  const auto test_some_optobj = OptObj::some(TestObject(4));
  const auto test_some_optobj2 = OptObj::some(TestObject(4));
  SOUL_TEST_RUN(test_clone_from(OptObj(), test_some_optobj));
  SOUL_TEST_RUN(test_clone_from(test_some_optobj, OptObj()));
  SOUL_TEST_RUN(test_clone_from(OptObj(), OptObj()));
  SOUL_TEST_RUN(test_clone_from(test_some_optobj, test_some_optobj2));

  const auto test_some_optlistobj =
    OptListObj::some(ListTestObject::generate_n(test_object_factory, 10));
  const auto test_some_optlistobj2 =
    OptListObj::some(ListTestObject::generate_n(test_object_factory, 3));
  SOUL_TEST_RUN(test_clone_from(OptListObj(), test_some_optlistobj));
  SOUL_TEST_RUN(test_clone_from(test_some_optlistobj, OptListObj()));
  SOUL_TEST_RUN(test_clone_from(OptListObj(), OptListObj()));
  SOUL_TEST_RUN(test_clone_from(test_some_optlistobj, test_some_optlistobj2));
}

template <typename T>
auto test_move_assignment(
  const soul::Option<T>& sample_opt_src, const soul::Option<T>& sample_opt_dst)
{
  auto opt_src = sample_opt_src.clone();
  auto opt_dst = sample_opt_dst.clone();
  opt_dst = std::move(opt_src);
  verify_option_equal<T>(opt_dst, sample_opt_src);
}

TEST(TestOptionAssignment, TestMoveAssignment)
{
  const auto test_some_optobj = OptObj::some(TestObject(4));
  const auto test_some_optobj2 = OptObj::some(TestObject(4));
  SOUL_TEST_RUN(test_move_assignment(OptObj(), test_some_optobj));
  SOUL_TEST_RUN(test_move_assignment(test_some_optobj, OptObj()));
  SOUL_TEST_RUN(test_move_assignment(OptObj(), OptObj()));
  SOUL_TEST_RUN(test_move_assignment(test_some_optobj, test_some_optobj2));

  const auto test_some_optlistobj =
    OptListObj::some(ListTestObject::generate_n(test_object_factory, 10));
  const auto test_some_optlistobj2 =
    OptListObj::some(ListTestObject::generate_n(test_object_factory, 3));
  SOUL_TEST_RUN(test_move_assignment(OptListObj(), test_some_optlistobj));
  SOUL_TEST_RUN(test_move_assignment(test_some_optlistobj, OptListObj()));
  SOUL_TEST_RUN(test_move_assignment(OptListObj(), OptListObj()));
  SOUL_TEST_RUN(test_move_assignment(test_some_optlistobj, test_some_optlistobj2));
}

TEST(TestOptionUnwrapOr, TestOptionUnwrapOr)
{
  {
    auto test_opt_some = OptInt::some(10);
    SOUL_TEST_ASSERT_EQ(test_opt_some.unwrap_or(5), 10);
  }

  {
    auto test_opt_none = OptInt();
    SOUL_TEST_ASSERT_EQ(test_opt_none.unwrap_or(3), 3);
  }

  SOUL_TEST_ASSERT_EQ(OptInt::some(3).unwrap_or(5), 3);
  SOUL_TEST_ASSERT_EQ(OptInt().unwrap_or(5), 5);

  SOUL_TEST_ASSERT_EQ(OptObj::some(TestObject(3)).unwrap_or(TestObject(5)), TestObject(3));
  SOUL_TEST_ASSERT_EQ(OptObj().unwrap_or(TestObject(5)), TestObject(5));

  const auto test_listobj1 = ListTestObject::generate_n(test_object_factory, 3);
  const auto test_listobj2 = ListTestObject::generate_n(test_object_factory, 10);
  SOUL_TEST_ASSERT_EQ(
    OptListObj::some(test_listobj1.clone()).unwrap_or(test_listobj2.clone()), test_listobj1);
  SOUL_TEST_ASSERT_EQ(OptListObj().unwrap_or(test_listobj2.clone()), test_listobj2);
}

TEST(TestOptionUnwrapOrElse, TestUnwrapOrElse)
{
  SOUL_TEST_ASSERT_EQ(OptInt::some(3).unwrap_or_else([] { return 5; }), 3);
  SOUL_TEST_ASSERT_EQ(OptInt().unwrap_or_else([]() { return 5; }), 5);

  SOUL_TEST_ASSERT_EQ(
    OptObj::some(TestObject(3)).unwrap_or_else([] { return TestObject(5); }), TestObject(3));
  SOUL_TEST_ASSERT_EQ(OptObj().unwrap_or_else([] { return TestObject(5); }), TestObject(5));

  const auto test_listobj1 = ListTestObject::generate_n(test_object_factory, 3);
  const auto test_listobj2 = ListTestObject::generate_n(test_object_factory, 10);
  SOUL_TEST_ASSERT_EQ(
    OptListObj::some(test_listobj1.clone()).unwrap_or_else(soul::clone_fn(test_listobj2)),
    test_listobj1);
  SOUL_TEST_ASSERT_EQ(OptListObj().unwrap_or_else(soul::clone_fn(test_listobj2)), test_listobj2);
}

TEST(TestOptionAndThen, TestAndThen)
{
  {
    const auto opt_int_none = OptInt();
    const auto result = opt_int_none.and_then([](const int val) { return OptInt::some(val + 1); });
    verify_option_equal(result, OptInt());
  }

  {
    const auto opt_some_listtestobj =
      OptListObj::some(ListTestObject::generate_n([] { return TestObject(5); }, 10));
    const auto result = opt_some_listtestobj.and_then(
      [](const ListTestObject& val) { return soul::Option<usize>::some(val.size()); });
    verify_option_equal(result, soul::Option<usize>::some(10));
  }

  {
    const auto result = OptObj::some(TestObject(10)).and_then([](TestObject&& test_object) {
      return OptListObj::some(
        ListTestObject::generate_n([&test_object] { return test_object.clone(); }, 10));
    });
    verify_option_equal(
      result, OptListObj::some(ListTestObject::generate_n([] { return TestObject(10); }, 10)));
  }
}

TEST(TestOptionTransform, TestTransform)
{
  {
    const auto opt_int_none = OptInt();
    const auto result = opt_int_none.transform([](const int val) { return val + 1; });
    verify_option_equal(result, OptInt());
  }

  {
    const auto opt_some_listtestobj =
      OptListObj::some(ListTestObject::generate_n([] { return TestObject(5); }, 10));
    const auto result =
      opt_some_listtestobj.transform([](const ListTestObject& val) { return val.size(); });
    verify_option_equal(result, soul::Option<usize>::some(10));
  }

  {
    auto generate_list_test_object = [](TestObject&& test_object) -> ListTestObject {
      return ListTestObject::generate_n([&test_object] { return test_object.clone(); }, 10);
    };
    const auto result = OptObj::some(TestObject(10)).transform(generate_list_test_object);
    verify_option_equal(
      result, OptListObj::some(ListTestObject::generate_n([] { return TestObject(10); }, 10)));
  }
}

TEST(TestOptionOrElse, TestOrElse)
{
  {
    const auto result = OptInt().or_else([] { return OptInt::some(3); });
    verify_option_equal(result, OptInt::some(3));
  }

  {
    const auto opt_int_some = OptInt::some(5);
    const auto result = opt_int_some.or_else([] { return OptInt::some(3); });
    verify_option_equal(result, OptInt::some(5));
  }

  {
    const auto expected_list_object = ListTestObject::from(
      std::views::iota(3, 10) | std::views::transform([](i32 val) { return TestObject(val); }));
    const auto expected_opt = OptListObj::some(expected_list_object.clone());
    const auto result = OptListObj().or_else([&] { return expected_opt.clone(); });
    verify_option_equal(result, expected_opt);
  }

  {
    const auto expected_list_object = ListTestObject::from(
      std::views::iota(3, 10) | std::views::transform([](i32 val) { return TestObject(val); }));
    const auto expected_opt = OptListObj::some(expected_list_object.clone());
    const auto result = expected_opt.clone().or_else([&] { return OptListObj(); });
    verify_option_equal(result, expected_opt);
  }
}

TEST(TestOptionReset, TestReset)
{
  auto opt_int_none = OptInt();
  opt_int_none.reset();
  verify_option_equal(opt_int_none, OptInt());

  auto opt_some_listtestobj =
    OptListObj::some(ListTestObject::generate_n([] { return TestObject(5); }, 10));
  opt_some_listtestobj.reset();
  verify_option_equal(opt_some_listtestobj, OptListObj());
}

template <typename T>
auto test_swap(const soul::Option<T>& sample_opt_lhs, const soul::Option<T>& sample_opt_rhs)
{
  auto opt_lhs = sample_opt_lhs.clone();
  auto opt_rhs = sample_opt_rhs.clone();
  swap(opt_lhs, opt_rhs);
  verify_option_equal<T>(opt_rhs, sample_opt_lhs);
  verify_option_equal<T>(opt_lhs, sample_opt_rhs);
}

TEST(TestOptionSwap, TestSwap)
{
  const auto test_some_optobj = OptObj::some(TestObject(4));
  const auto test_some_optobj2 = OptObj::some(TestObject(4));
  SOUL_TEST_RUN(test_swap(OptObj(), test_some_optobj));
  SOUL_TEST_RUN(test_swap(test_some_optobj, OptObj()));
  SOUL_TEST_RUN(test_swap(OptObj(), OptObj()));
  SOUL_TEST_RUN(test_swap(test_some_optobj, test_some_optobj2));

  const auto test_some_optlistobj =
    OptListObj::some(ListTestObject::generate_n(test_object_factory, 10));
  const auto test_some_optlistobj2 =
    OptListObj::some(ListTestObject::generate_n(test_object_factory, 3));
  SOUL_TEST_RUN(test_swap(OptListObj(), test_some_optlistobj));
  SOUL_TEST_RUN(test_swap(test_some_optlistobj, OptListObj()));
  SOUL_TEST_RUN(test_swap(OptListObj(), OptListObj()));
  SOUL_TEST_RUN(test_swap(test_some_optlistobj, test_some_optlistobj2));
}
