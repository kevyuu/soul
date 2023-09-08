#include <gtest/gtest.h>

#include "core/config.h"
#include "core/objops.h"
#include "core/result.h"
#include "core/type_traits.h"
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

using namespace soul;

struct TrivialOk {
  int x, y;
  auto operator==(const TrivialOk&) const -> bool = default;
};
using TrivialErr = u64;
using NontrivialErr = ListTestObject;

using TrivialResult = soul::Result<TrivialOk, TrivialErr>;
using TrivialOkResult = soul::Result<TrivialOk, TestObject>;
using TrivialErrResult = soul::Result<TestObject, TrivialErr>;
using NontrivialResult = soul::Result<TestObject, ListTestObject>;

template <soul::typeset ResultT, soul::typeset T>
auto test_construction_ok(const T& ok_src)
{
  auto result = ResultT::ok(duplicate(ok_src));
  SOUL_TEST_ASSERT_EQ(result.ok_ref(), ok_src);
  SOUL_TEST_ASSERT_TRUE(result.is_ok());
  SOUL_TEST_ASSERT_FALSE(result.is_err());
  if constexpr (soul::can_trivial_copy_v<T>) {
    SOUL_TEST_ASSERT_EQ(result.unwrap(), ok_src);
  }
  SOUL_TEST_ASSERT_EQ(std::move(result).unwrap(), ok_src);
}

TEST(TestResultConstruction, TestOkConstruction)
{
  test_construction_ok<TrivialResult>(TrivialOk{.x = 3, .y = 10});
  test_construction_ok<TrivialOkResult>(TrivialOk{.x = 3, .y = 10});
  test_construction_ok<TrivialErrResult>(TestObject(10));
  test_construction_ok<NontrivialResult>(TestObject(10));
}

template <soul::typeset ResultT, soul::typeset T>
auto test_construction_err(const T& err_src)
{
  auto result = ResultT::err(duplicate(err_src));
  SOUL_TEST_ASSERT_EQ(result.err_ref(), err_src);
  SOUL_TEST_ASSERT_FALSE(result.is_ok());
  SOUL_TEST_ASSERT_TRUE(result.is_err());
}

TEST(TestResultConstruction, TestErrConstruction)
{
  const auto dummy_nontrivial_err = NontrivialErr::From(
    std::views::iota(3, 10) | std::views::transform([](i32 val) { return TestObject(val); }));

  test_construction_err<TrivialResult>(TrivialErr(3));
  test_construction_err<TrivialOkResult>(TestObject(10));
  test_construction_err<TrivialErrResult>(TrivialErr(4));
  test_construction_err<NontrivialResult>(dummy_nontrivial_err);
}

template <soul::typeset ResultT, soul::typeset Fn>
auto test_construction_generate(Fn fn)
{
  auto ok_src = std::invoke(fn);
  auto result = ResultT::init_generate(fn);
  SOUL_TEST_ASSERT_EQ(result.ok_ref(), ok_src);
  SOUL_TEST_ASSERT_TRUE(result.is_ok());
  SOUL_TEST_ASSERT_FALSE(result.is_err());
  if constexpr (soul::can_trivial_copy_v<decltype(ok_src)>) {
    SOUL_TEST_ASSERT_EQ(result.unwrap(), ok_src);
  }
  SOUL_TEST_ASSERT_EQ(std::move(result).unwrap(), ok_src);
}

TEST(TestResultConstruction, TestConstructionGenerateOk)
{
  test_construction_generate<TrivialResult>([] { return TrivialOk{.x = 3, .y = 10}; });
  test_construction_generate<TrivialOkResult>([] { return TrivialOk{.x = 3, .y = 10}; });
  test_construction_generate<TrivialErrResult>([] { return TestObject(10); });
  test_construction_generate<NontrivialResult>([] { return TestObject(10); });
}

template <soul::typeset ResultT, soul::typeset Fn>
auto test_construction_generate_err(Fn fn)
{
  auto err_src = std::invoke(fn);
  auto result = ResultT::init_generate_err(fn);
  SOUL_TEST_ASSERT_EQ(result.err_ref(), err_src);
  SOUL_TEST_ASSERT_FALSE(result.is_ok());
  SOUL_TEST_ASSERT_TRUE(result.is_err());
}

TEST(TestResultConstruction, TestConstructionGenerateErr)
{
  const auto dummy_nontrivial_err = NontrivialErr::From(
    std::views::iota(3, 10) | std::views::transform([](i32 val) { return TestObject(val); }));

  test_construction_generate_err<TrivialResult>(duplicate_fn(TrivialErr(3)));
  test_construction_generate_err<TrivialOkResult>(duplicate_fn(TestObject(10)));
  test_construction_generate_err<TrivialErrResult>(duplicate_fn(TrivialErr(4)));
  test_construction_generate_err<NontrivialResult>(clone_fn(dummy_nontrivial_err));
}

template <ts_result ResultT>
void verify_equal(const ResultT& lhs, const ResultT& rhs)
{
  SOUL_TEST_ASSERT_EQ(lhs.is_ok(), rhs.is_ok());
  if (lhs.is_ok() && rhs.is_ok()) {
    SOUL_TEST_ASSERT_EQ(lhs.ok_ref(), rhs.ok_ref());
  } else if (lhs.is_err() && rhs.is_err()) {
    SOUL_TEST_ASSERT_EQ(lhs.err_ref(), rhs.err_ref());
  }
  SOUL_TEST_ASSERT_EQ(lhs, rhs);
}

TEST(TestResultConstruction, TestCopyConstructor)
{
  test_copy_constructor(TrivialResult::ok(TrivialOk{.x = 3, .y = 4}));
  test_copy_constructor(TrivialResult::err(TrivialErr(3)));
}

TEST(TestResultConstruction, TestClone)
{
  const auto dummy_nontrivial_err = NontrivialErr::From(
    std::views::iota(3, 10) | std::views::transform([](i32 val) { return TestObject(val); }));

  test_clone(TrivialOkResult::ok(TrivialOk{.x = 3, .y = 10}));
  test_clone(TrivialErrResult::ok(TestObject(10)));
  test_clone(NontrivialResult::ok(TestObject(10)));

  test_clone(TrivialOkResult::err(TestObject(10)));
  test_clone(TrivialErrResult::err(TrivialErr(4)));
  test_clone(NontrivialResult::err(dummy_nontrivial_err.clone()));
}

TEST(TestResultConstruction, TestMoveConstructor)
{
  const auto dummy_nontrivial_err = NontrivialErr::From(
    std::views::iota(3, 10) | std::views::transform([](i32 val) { return TestObject(val); }));

  test_move_constructor(TrivialResult::ok(TrivialOk{.x = 3, .y = 10}));
  test_move_constructor(TrivialOkResult::ok(TrivialOk{.x = 3, .y = 10}));
  test_move_constructor(TrivialErrResult::ok(TestObject(10)));
  test_move_constructor(NontrivialResult::ok(TestObject(10)));

  test_move_constructor(TrivialResult::err(TrivialErr(9)));
  test_move_constructor(TrivialOkResult::err(TestObject(10)));
  test_move_constructor(TrivialErrResult::err(TrivialErr(4)));
  test_move_constructor(NontrivialResult::err(dummy_nontrivial_err.clone()));
}

class TestResultManipulation : public testing::Test
{
public:
  TrivialOk trivial_ok = TrivialOk{.x = 3, .y = 10};
  TrivialOk trivial_ok2 = TrivialOk{.x = 7, .y = 8};
  TrivialErr trivial_err = TrivialErr(2);
  TrivialErr trivial_err2 = TrivialErr(5);

  TestObject test_obj = TestObject(10);
  TestObject test_obj2 = TestObject(7);

  NontrivialErr nontrivial_err = NontrivialErr::From(
    std::views::iota(3, 10) | std::views::transform([](i32 val) { return TestObject(val); }));
  NontrivialErr nontrivial_err2 = NontrivialErr::From(
    std::views::iota(3, 7) | std::views::transform([](i32 val) { return TestObject(val); }));

  TrivialResult trivial_result_ok = TrivialResult::ok(trivial_ok);
  TrivialResult trivial_result_ok2 = TrivialResult::ok(trivial_ok2);
  TrivialResult trivial_result_err = TrivialResult::err(trivial_err);
  TrivialResult trivial_result_err2 = TrivialResult::err(trivial_err2);

  TrivialOkResult trivial_ok_result_ok = TrivialOkResult::ok(trivial_ok);
  TrivialOkResult trivial_ok_result_ok2 = TrivialOkResult::ok(trivial_ok2);
  TrivialOkResult trivial_ok_result_err = TrivialOkResult::err(test_obj.clone());
  TrivialOkResult trivial_ok_result_err2 = TrivialOkResult::err(test_obj2.clone());

  TrivialErrResult trivial_err_result_ok = TrivialErrResult::ok(test_obj.clone());
  TrivialErrResult trivial_err_result_ok2 = TrivialErrResult::ok(test_obj2.clone());
  TrivialErrResult trivial_err_result_err = TrivialErrResult::err(trivial_err);
  TrivialErrResult trivial_err_result_err2 = TrivialErrResult::err(trivial_err2);

  NontrivialResult nontrivial_result_ok = NontrivialResult::ok(test_obj.clone());
  NontrivialResult nontrivial_result_ok2 = NontrivialResult::ok(test_obj2.clone());
  NontrivialResult nontrivial_result_err = NontrivialResult::err(nontrivial_err.clone());
  NontrivialResult nontrivial_result_err2 = NontrivialResult::err(nontrivial_err2.clone());
};

TEST_F(TestResultManipulation, TestCopyAssignment)
{
  SOUL_TEST_RUN(test_copy_assignment(trivial_result_ok, trivial_result_ok2));
  SOUL_TEST_RUN(test_copy_assignment(trivial_result_ok, trivial_result_err));
  SOUL_TEST_RUN(test_copy_assignment(trivial_result_err, trivial_result_ok));
  SOUL_TEST_RUN(test_copy_assignment(trivial_result_err, trivial_result_err2));
}

TEST_F(TestResultManipulation, TestMoveAssignment)
{

  SOUL_TEST_RUN(test_move_assignment(trivial_ok_result_ok, trivial_ok_result_ok2));
  SOUL_TEST_RUN(test_move_assignment(trivial_ok_result_ok, trivial_ok_result_err));
  SOUL_TEST_RUN(test_move_assignment(trivial_ok_result_err, trivial_ok_result_ok));
  SOUL_TEST_RUN(test_move_assignment(trivial_ok_result_err, trivial_ok_result_err2));

  SOUL_TEST_RUN(test_move_assignment(trivial_err_result_ok, trivial_err_result_ok2));
  SOUL_TEST_RUN(test_move_assignment(trivial_err_result_ok, trivial_err_result_err));
  SOUL_TEST_RUN(test_move_assignment(trivial_err_result_err, trivial_err_result_ok));
  SOUL_TEST_RUN(test_move_assignment(trivial_err_result_err, trivial_err_result_err2));

  SOUL_TEST_RUN(test_move_assignment(nontrivial_result_ok, nontrivial_result_ok2));
  SOUL_TEST_RUN(test_move_assignment(nontrivial_result_ok, nontrivial_result_err));
  SOUL_TEST_RUN(test_move_assignment(nontrivial_result_err, nontrivial_result_ok));
  SOUL_TEST_RUN(test_move_assignment(nontrivial_result_err, nontrivial_result_err2));
}

TEST_F(TestResultManipulation, TestCloneFrom)
{

  SOUL_TEST_RUN(test_clone_from(trivial_ok_result_ok, trivial_ok_result_ok2));
  SOUL_TEST_RUN(test_clone_from(trivial_ok_result_ok, trivial_ok_result_err));
  SOUL_TEST_RUN(test_clone_from(trivial_ok_result_err, trivial_ok_result_ok));
  SOUL_TEST_RUN(test_clone_from(trivial_ok_result_err, trivial_ok_result_err2));

  SOUL_TEST_RUN(test_clone_from(trivial_err_result_ok, trivial_err_result_ok2));
  SOUL_TEST_RUN(test_clone_from(trivial_err_result_ok, trivial_err_result_err));
  SOUL_TEST_RUN(test_clone_from(trivial_err_result_err, trivial_err_result_ok));
  SOUL_TEST_RUN(test_clone_from(trivial_err_result_err, trivial_err_result_err2));

  SOUL_TEST_RUN(test_clone_from(nontrivial_result_ok, nontrivial_result_ok2));
  SOUL_TEST_RUN(test_clone_from(nontrivial_result_ok, nontrivial_result_err));
  SOUL_TEST_RUN(test_clone_from(nontrivial_result_err, nontrivial_result_ok));
  SOUL_TEST_RUN(test_clone_from(nontrivial_result_err, nontrivial_result_err2));
}

TEST_F(TestResultManipulation, TestSwap)
{
  SOUL_TEST_RUN(test_swap(trivial_result_ok, trivial_result_ok2));
  SOUL_TEST_RUN(test_swap(trivial_result_ok, trivial_result_err));
  SOUL_TEST_RUN(test_swap(trivial_result_err, trivial_result_ok));
  SOUL_TEST_RUN(test_swap(trivial_result_err, trivial_result_err2));

  SOUL_TEST_RUN(test_swap(trivial_ok_result_ok, trivial_ok_result_ok2));
  SOUL_TEST_RUN(test_swap(trivial_ok_result_ok, trivial_ok_result_err));
  SOUL_TEST_RUN(test_swap(trivial_ok_result_err, trivial_ok_result_ok));
  SOUL_TEST_RUN(test_swap(trivial_ok_result_err, trivial_ok_result_err2));

  SOUL_TEST_RUN(test_swap(trivial_err_result_ok, trivial_err_result_ok2));
  SOUL_TEST_RUN(test_swap(trivial_err_result_ok, trivial_err_result_err));
  SOUL_TEST_RUN(test_swap(trivial_err_result_err, trivial_err_result_ok));
  SOUL_TEST_RUN(test_swap(trivial_err_result_err, trivial_err_result_err2));

  SOUL_TEST_RUN(test_swap(nontrivial_result_ok, nontrivial_result_ok2));
  SOUL_TEST_RUN(test_swap(nontrivial_result_ok, nontrivial_result_err));
  SOUL_TEST_RUN(test_swap(nontrivial_result_err, nontrivial_result_ok));
  SOUL_TEST_RUN(test_swap(nontrivial_result_err, nontrivial_result_err2));
}

TEST_F(TestResultManipulation, TestUnwrapOr)
{
  const auto trivial_ok_default = TrivialOk{.x = 100, .y = 37};
  SOUL_TEST_ASSERT_EQ(trivial_result_ok.unwrap_or(trivial_ok_default), trivial_ok);
  SOUL_TEST_ASSERT_EQ(trivial_result_err.unwrap_or(trivial_ok_default), trivial_ok_default);

  SOUL_TEST_ASSERT_EQ(trivial_ok_result_ok.unwrap_or(trivial_ok_default), trivial_ok);
  SOUL_TEST_ASSERT_EQ(trivial_ok_result_err.unwrap_or(trivial_ok_default), trivial_ok_default);

  const auto test_obj_default = TestObject(37);
  SOUL_TEST_ASSERT_EQ(trivial_err_result_ok.clone().unwrap_or(test_obj_default.clone()), test_obj);
  SOUL_TEST_ASSERT_EQ(
    trivial_err_result_err.clone().unwrap_or(test_obj_default.clone()), test_obj_default);

  SOUL_TEST_ASSERT_EQ(nontrivial_result_ok.clone().unwrap_or(test_obj_default.clone()), test_obj);
  SOUL_TEST_ASSERT_EQ(
    nontrivial_result_err.clone().unwrap_or(test_obj_default.clone()), test_obj_default);
}

TEST_F(TestResultManipulation, TestUnwrapOrElse)
{
  const auto trivial_ok_default = TrivialOk{.x = 100, .y = 37};
  const auto trivial_ok_fn_default = duplicate_fn(trivial_ok_default);
  SOUL_TEST_ASSERT_EQ(trivial_result_ok.unwrap_or_else(trivial_ok_fn_default), trivial_ok);
  SOUL_TEST_ASSERT_EQ(trivial_result_err.unwrap_or_else(trivial_ok_fn_default), trivial_ok_default);

  SOUL_TEST_ASSERT_EQ(trivial_ok_result_ok.unwrap_or_else(trivial_ok_fn_default), trivial_ok);
  SOUL_TEST_ASSERT_EQ(
    trivial_ok_result_err.unwrap_or_else(trivial_ok_fn_default), trivial_ok_default);

  const auto test_obj_default = TestObject(37);
  const auto test_obj_fn_default = clone_fn(test_obj_default);
  SOUL_TEST_ASSERT_EQ(trivial_err_result_ok.clone().unwrap_or_else(test_obj_fn_default), test_obj);
  SOUL_TEST_ASSERT_EQ(
    trivial_err_result_err.clone().unwrap_or_else(test_obj_fn_default), test_obj_default);

  SOUL_TEST_ASSERT_EQ(nontrivial_result_ok.clone().unwrap_or_else(test_obj_fn_default), test_obj);
  SOUL_TEST_ASSERT_EQ(
    nontrivial_result_err.clone().unwrap_or_else(test_obj_fn_default), test_obj_default);
}

TEST_F(TestResultManipulation, TestAndThen)
{
  {
    const auto and_then_fn = [&](const TrivialOk& val) {
      return Result<int, TrivialErr>::ok(val.x);
    };
    const auto expected_when_ok = and_then_fn(trivial_ok);
    const auto expected_when_err = Result<int, TrivialErr>::err(trivial_err);
    SOUL_TEST_ASSERT_EQ(trivial_result_ok.and_then(and_then_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(trivial_result_err.and_then(and_then_fn), expected_when_err);
  }

  {
    const auto and_then_fn = [&](const TrivialOk& val) {
      return Result<int, TestObject>::ok(val.x);
    };
    const auto expected_when_ok = and_then_fn(trivial_ok);
    const auto expected_when_err = Result<int, TestObject>::err(test_obj.clone());
    SOUL_TEST_ASSERT_EQ(trivial_ok_result_ok.clone().and_then(and_then_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(trivial_ok_result_err.clone().and_then(and_then_fn), expected_when_err);
  }

  {
    const auto and_then_fn = [&](const TestObject& val) {
      return Result<int, TrivialErr>::ok(val.x);
    };
    const auto expected_when_ok = and_then_fn(test_obj);
    const auto expected_when_err = Result<int, TrivialErr>::err(trivial_err);
    SOUL_TEST_ASSERT_EQ(trivial_err_result_ok.and_then(and_then_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(trivial_err_result_err.clone().and_then(and_then_fn), expected_when_err);
  }

  {
    const auto and_then_fn = [&](const TestObject& val) {
      return Result<int, NontrivialErr>::ok(val.x);
    };
    const auto expected_when_ok = and_then_fn(test_obj);
    const auto expected_when_err = Result<int, NontrivialErr>::err(nontrivial_err.clone());
    SOUL_TEST_ASSERT_EQ(nontrivial_result_ok.clone().and_then(and_then_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(nontrivial_result_err.clone().and_then(and_then_fn), expected_when_err);
  }
}

TEST_F(TestResultManipulation, TestTransform)
{
  {
    using ExpectedResult = Result<int, TrivialErr>;
    const auto transform_fn = [&](const TrivialOk& val) { return val.x; };
    const auto expected_when_ok = ExpectedResult::ok(transform_fn(trivial_ok));
    const auto expected_when_err = ExpectedResult::err(trivial_err);
    SOUL_TEST_ASSERT_EQ(trivial_result_ok.transform(transform_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(trivial_result_err.transform(transform_fn), expected_when_err);
  }

  {
    using ExpectedResult = Result<int, TestObject>;
    const auto transform_fn = [&](const TrivialOk& val) { return val.x; };
    const auto expected_when_ok = ExpectedResult::ok(transform_fn(trivial_ok));
    const auto expected_when_err = ExpectedResult::err(test_obj.clone());
    SOUL_TEST_ASSERT_EQ(trivial_ok_result_ok.clone().transform(transform_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(trivial_ok_result_err.clone().transform(transform_fn), expected_when_err);
  }

  {
    using ExpectedResult = Result<int, TrivialErr>;
    const auto transform_fn = [&](const TestObject& val) { return val.x; };
    const auto expected_when_ok = ExpectedResult::ok(transform_fn(test_obj));
    const auto expected_when_err = Result<int, TrivialErr>::err(trivial_err);
    SOUL_TEST_ASSERT_EQ(trivial_err_result_ok.transform(transform_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(trivial_err_result_err.clone().transform(transform_fn), expected_when_err);
  }

  {
    using ExpectedResult = Result<int, NontrivialErr>;
    const auto transform_fn = [&](const TestObject& val) { return val.x; };
    const auto expected_when_ok = ExpectedResult::ok(transform_fn(test_obj));
    const auto expected_when_err = ExpectedResult::err(nontrivial_err.clone());
    SOUL_TEST_ASSERT_EQ(nontrivial_result_ok.clone().transform(transform_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(nontrivial_result_err.clone().transform(transform_fn), expected_when_err);
  }
}

TEST_F(TestResultManipulation, TestOrElse)
{
  struct OrElseError {
    TrivialErr x;
    TrivialErr y;
    auto operator==(const OrElseError&) const -> bool = default;
  };

  {
    using ExpectedResult = Result<TrivialOk, OrElseError>;
    const auto or_else_fn = [&](const TrivialErr& val) {
      return ExpectedResult::err(OrElseError{val, val});
    };
    const auto expected_when_ok = ExpectedResult::ok(trivial_ok);
    const auto expected_when_err = or_else_fn(trivial_err);
    SOUL_TEST_ASSERT_EQ(trivial_result_ok.or_else(or_else_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(trivial_result_err.or_else(or_else_fn), expected_when_err);
  }

  {
    using ExpectedResult = Result<TestObject, OrElseError>;
    const auto or_else_fn = [&](const NontrivialErr& val) {
      return ExpectedResult::err(OrElseError{val.size(), val.size()});
    };
    const auto expected_when_ok = ExpectedResult::ok(test_obj.clone());
    const auto expected_when_err = or_else_fn(nontrivial_err);
    SOUL_TEST_ASSERT_EQ(nontrivial_result_ok.clone().or_else(or_else_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(nontrivial_result_err.clone().or_else(or_else_fn), expected_when_err);
  }
}

TEST(TestResultGetter, TestIsOkAnd)
{
  SOUL_TEST_ASSERT_TRUE(
    TrivialResult::ok(TrivialOk{.x = 7, .y = 6}).is_ok_and([](const TrivialOk& trivial_ok) {
      return trivial_ok.x == 7;
    }));

  SOUL_TEST_ASSERT_FALSE(
    TrivialResult::ok(TrivialOk{.x = 7, .y = 6}).is_ok_and([](const TrivialOk& trivial_ok) {
      return trivial_ok.x == 5;
    }));

  SOUL_TEST_ASSERT_FALSE(TrivialResult::err(10).is_ok_and(
    [](const TrivialOk& trivial_ok) -> b8 { return trivial_ok.x == 5; }));
}

TEST(TestResultGetter, TestIsErrAnd)
{
  SOUL_TEST_ASSERT_FALSE(
    TrivialResult::ok(TrivialOk{.x = 7, .y = 6}).is_err_and([](const TrivialErr trivial_err) {
      return trivial_err == 7;
    }));

  SOUL_TEST_ASSERT_FALSE(
    TrivialResult::err(TrivialErr(5)).is_err_and([](const TrivialErr& trivial_err) {
      return trivial_err == 8;
    }));

  SOUL_TEST_ASSERT_TRUE(
    TrivialResult::err(TrivialErr(5)).is_err_and([](const TrivialErr& trivial_err) {
      return trivial_err == 5;
    }));
}
