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
  auto result = ResultT::Ok(duplicate(ok_src));
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
  auto result = ResultT::Err(duplicate(err_src));
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
  auto result = ResultT::Generate(fn);
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
  auto result = ResultT::GenerateErr(fn);
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
  test_copy_constructor(TrivialResult::Ok(TrivialOk{.x = 3, .y = 4}));
  test_copy_constructor(TrivialResult::Err(TrivialErr(3)));
}

TEST(TestResultConstruction, TestClone)
{
  const auto dummy_nontrivial_err = NontrivialErr::From(
    std::views::iota(3, 10) | std::views::transform([](i32 val) { return TestObject(val); }));

  test_clone(TrivialOkResult::Ok(TrivialOk{.x = 3, .y = 10}));
  test_clone(TrivialErrResult::Ok(TestObject(10)));
  test_clone(NontrivialResult::Ok(TestObject(10)));

  test_clone(TrivialOkResult::Err(TestObject(10)));
  test_clone(TrivialErrResult::Err(TrivialErr(4)));
  test_clone(NontrivialResult::Err(dummy_nontrivial_err.clone()));
}

TEST(TestResultConstruction, TestMoveConstructor)
{
  const auto dummy_nontrivial_err = NontrivialErr::From(
    std::views::iota(3, 10) | std::views::transform([](i32 val) { return TestObject(val); }));

  test_move_constructor(TrivialResult::Ok(TrivialOk{.x = 3, .y = 10}));
  test_move_constructor(TrivialOkResult::Ok(TrivialOk{.x = 3, .y = 10}));
  test_move_constructor(TrivialErrResult::Ok(TestObject(10)));
  test_move_constructor(NontrivialResult::Ok(TestObject(10)));

  test_move_constructor(TrivialResult::Err(TrivialErr(9)));
  test_move_constructor(TrivialOkResult::Err(TestObject(10)));
  test_move_constructor(TrivialErrResult::Err(TrivialErr(4)));
  test_move_constructor(NontrivialResult::Err(dummy_nontrivial_err.clone()));
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

  TrivialResult trivial_result_ok = TrivialResult::Ok(trivial_ok);
  TrivialResult trivial_result_ok2 = TrivialResult::Ok(trivial_ok2);
  TrivialResult trivial_result_err = TrivialResult::Err(trivial_err);
  TrivialResult trivial_result_err2 = TrivialResult::Err(trivial_err2);

  TrivialOkResult trivial_ok_result_ok = TrivialOkResult::Ok(trivial_ok);
  TrivialOkResult trivial_ok_result_ok2 = TrivialOkResult::Ok(trivial_ok2);
  TrivialOkResult trivial_ok_result_err = TrivialOkResult::Err(test_obj.clone());
  TrivialOkResult trivial_ok_result_err2 = TrivialOkResult::Err(test_obj2.clone());

  TrivialErrResult trivial_err_result_ok = TrivialErrResult::Ok(test_obj.clone());
  TrivialErrResult trivial_err_result_ok2 = TrivialErrResult::Ok(test_obj2.clone());
  TrivialErrResult trivial_err_result_err = TrivialErrResult::Err(trivial_err);
  TrivialErrResult trivial_err_result_err2 = TrivialErrResult::Err(trivial_err2);

  NontrivialResult nontrivial_result_ok = NontrivialResult::Ok(test_obj.clone());
  NontrivialResult nontrivial_result_ok2 = NontrivialResult::Ok(test_obj2.clone());
  NontrivialResult nontrivial_result_err = NontrivialResult::Err(nontrivial_err.clone());
  NontrivialResult nontrivial_result_err2 = NontrivialResult::Err(nontrivial_err2.clone());
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
      return Result<int, TrivialErr>::Ok(val.x);
    };
    const auto expected_when_ok = and_then_fn(trivial_ok);
    const auto expected_when_err = Result<int, TrivialErr>::Err(trivial_err);
    SOUL_TEST_ASSERT_EQ(trivial_result_ok.and_then(and_then_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(trivial_result_err.and_then(and_then_fn), expected_when_err);
  }

  {
    const auto and_then_fn = [&](const TrivialOk& val) {
      return Result<int, TestObject>::Ok(val.x);
    };
    const auto expected_when_ok = and_then_fn(trivial_ok);
    const auto expected_when_err = Result<int, TestObject>::Err(test_obj.clone());
    SOUL_TEST_ASSERT_EQ(trivial_ok_result_ok.clone().and_then(and_then_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(trivial_ok_result_err.clone().and_then(and_then_fn), expected_when_err);
  }

  {
    const auto and_then_fn = [&](const TestObject& val) {
      return Result<int, TrivialErr>::Ok(val.x);
    };
    const auto expected_when_ok = and_then_fn(test_obj);
    const auto expected_when_err = Result<int, TrivialErr>::Err(trivial_err);
    SOUL_TEST_ASSERT_EQ(trivial_err_result_ok.and_then(and_then_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(trivial_err_result_err.clone().and_then(and_then_fn), expected_when_err);
  }

  {
    const auto and_then_fn = [&](const TestObject& val) {
      return Result<int, NontrivialErr>::Ok(val.x);
    };
    const auto expected_when_ok = and_then_fn(test_obj);
    const auto expected_when_err = Result<int, NontrivialErr>::Err(nontrivial_err.clone());
    SOUL_TEST_ASSERT_EQ(nontrivial_result_ok.clone().and_then(and_then_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(nontrivial_result_err.clone().and_then(and_then_fn), expected_when_err);
  }
}

TEST_F(TestResultManipulation, TestTransform)
{
  {
    using ExpectedResult = Result<int, TrivialErr>;
    const auto transform_fn = [&](const TrivialOk& val) { return val.x; };
    const auto expected_when_ok = ExpectedResult::Ok(transform_fn(trivial_ok));
    const auto expected_when_err = ExpectedResult::Err(trivial_err);
    SOUL_TEST_ASSERT_EQ(trivial_result_ok.transform(transform_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(trivial_result_err.transform(transform_fn), expected_when_err);
  }

  {
    using ExpectedResult = Result<int, TestObject>;
    const auto transform_fn = [&](const TrivialOk& val) { return val.x; };
    const auto expected_when_ok = ExpectedResult::Ok(transform_fn(trivial_ok));
    const auto expected_when_err = ExpectedResult::Err(test_obj.clone());
    SOUL_TEST_ASSERT_EQ(trivial_ok_result_ok.clone().transform(transform_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(trivial_ok_result_err.clone().transform(transform_fn), expected_when_err);
  }

  {
    using ExpectedResult = Result<int, TrivialErr>;
    const auto transform_fn = [&](const TestObject& val) { return val.x; };
    const auto expected_when_ok = ExpectedResult::Ok(transform_fn(test_obj));
    const auto expected_when_err = Result<int, TrivialErr>::Err(trivial_err);
    SOUL_TEST_ASSERT_EQ(trivial_err_result_ok.transform(transform_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(trivial_err_result_err.clone().transform(transform_fn), expected_when_err);
  }

  {
    using ExpectedResult = Result<int, NontrivialErr>;
    const auto transform_fn = [&](const TestObject& val) { return val.x; };
    const auto expected_when_ok = ExpectedResult::Ok(transform_fn(test_obj));
    const auto expected_when_err = ExpectedResult::Err(nontrivial_err.clone());
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
      return ExpectedResult::Err(OrElseError{val, val});
    };
    const auto expected_when_ok = ExpectedResult::Ok(trivial_ok);
    const auto expected_when_err = or_else_fn(trivial_err);
    SOUL_TEST_ASSERT_EQ(trivial_result_ok.or_else(or_else_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(trivial_result_err.or_else(or_else_fn), expected_when_err);
  }

  {
    using ExpectedResult = Result<TestObject, OrElseError>;
    const auto or_else_fn = [&](const NontrivialErr& val) {
      return ExpectedResult::Err(OrElseError{val.size(), val.size()});
    };
    const auto expected_when_ok = ExpectedResult::Ok(test_obj.clone());
    const auto expected_when_err = or_else_fn(nontrivial_err);
    SOUL_TEST_ASSERT_EQ(nontrivial_result_ok.clone().or_else(or_else_fn), expected_when_ok);
    SOUL_TEST_ASSERT_EQ(nontrivial_result_err.clone().or_else(or_else_fn), expected_when_err);
  }
}

TEST(TestResultGetter, TestIsOkAnd)
{
  SOUL_TEST_ASSERT_TRUE(
    TrivialResult::Ok(TrivialOk{.x = 7, .y = 6}).is_ok_and([](const TrivialOk& trivial_ok) {
      return trivial_ok.x == 7;
    }));

  SOUL_TEST_ASSERT_FALSE(
    TrivialResult::Ok(TrivialOk{.x = 7, .y = 6}).is_ok_and([](const TrivialOk& trivial_ok) {
      return trivial_ok.x == 5;
    }));

  SOUL_TEST_ASSERT_FALSE(TrivialResult::Err(10).is_ok_and(
    [](const TrivialOk& trivial_ok) -> b8 { return trivial_ok.x == 5; }));
}

TEST(TestResultGetter, TestIsErrAnd)
{
  SOUL_TEST_ASSERT_FALSE(
    TrivialResult::Ok(TrivialOk{.x = 7, .y = 6}).is_err_and([](const TrivialErr trivial_err) {
      return trivial_err == 7;
    }));

  SOUL_TEST_ASSERT_FALSE(
    TrivialResult::Err(TrivialErr(5)).is_err_and([](const TrivialErr& trivial_err) {
      return trivial_err == 8;
    }));

  SOUL_TEST_ASSERT_TRUE(
    TrivialResult::Err(TrivialErr(5)).is_err_and([](const TrivialErr& trivial_err) {
      return trivial_err == 5;
    }));
}
