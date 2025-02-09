#include <gtest/gtest.h>

#include "core/config.h"
#include "core/hash.h"
#include "core/meta.h"
#include "core/objops.h"
#include "core/panic.h"
#include "core/type_traits.h"
#include "core/variant.h"
#include "core/views.h"
#include "memory/allocator.h"

#include "common_test.h"
#include "util.h"

namespace soul
{
  auto get_default_allocator() -> memory::Allocator*
  {
    static TestAllocator test_allocator("Test default allocator"_str);
    return &test_allocator;
  }
} // namespace soul

struct TrivialObj
{
  u8 x;
  u8 y;

  friend constexpr auto operator<=>(const TrivialObj& lhs, const TrivialObj& rhs) = default;

  friend constexpr void soul_op_hash_combine(auto& hasher, const TrivialObj& obj)
  {
    hasher.combine(obj.x, obj.y);
  }
};

struct MoveOnlyObj
{
  u8 x;
  u8 y;

  MoveOnlyObj(const MoveOnlyObj& other)                        = delete;
  MoveOnlyObj(MoveOnlyObj&& other)                             = default;
  auto operator=(const MoveOnlyObj& other)                     = delete;
  auto operator=(MoveOnlyObj&& other) noexcept -> MoveOnlyObj& = default;
  ~MoveOnlyObj()                                               = default;
};

using TrivialVariant = soul::Variant<u8, u16, TrivialObj>;
static_assert(can_trivial_move_v<TrivialVariant>);
static_assert(std::is_trivially_copyable_v<TrivialVariant>);
using ListTestObject   = soul::Vector<TestObject>;
using UntrivialVariant = soul::Variant<ListTestObject, TestObject, u8>;
static_assert(ts_clone<UntrivialVariant>);

using MoveOnlyVariant = soul::Variant<TestObject, u8, MoveOnlyObj>;
static_assert(ts_move_only<MoveOnlyVariant>);

TEST(TestVariantConstruction, TestConstructionFromValue)
{
  {
    const auto test_variant = TrivialVariant::From(u16(20));
    SOUL_TEST_ASSERT_EQ(test_variant.ref<u16>(), 20);
    SOUL_TEST_ASSERT_TRUE(test_variant.has_value<u16>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<u8>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<TrivialObj>());
  }

  {
    const auto test_trivial_obj = TrivialObj{.x = 30};
    const auto test_variant     = TrivialVariant::From(test_trivial_obj);
    SOUL_TEST_ASSERT_EQ(test_variant.ref<TrivialObj>().x, 30);
    SOUL_TEST_ASSERT_EQ(test_variant.ref<TrivialObj>().y, 0);
    SOUL_TEST_ASSERT_TRUE(test_variant.has_value<TrivialObj>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<u8>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<u16>());
  }

  {
    const auto test_variant = UntrivialVariant::From(TestObject(3));
    SOUL_TEST_ASSERT_EQ(test_variant.ref<TestObject>().x, 3);
    SOUL_TEST_ASSERT_TRUE(test_variant.has_value<TestObject>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<ListTestObject>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<u8>());
  }

  {
    const auto test_variant = UntrivialVariant::From(ListTestObject::From(
      std::views::iota(0, 10) | std::views::transform(
                                  [](int i)
                                  {
                                    return TestObject(i);
                                  })));
    SOUL_TEST_ASSERT_EQ(test_variant.ref<ListTestObject>().size(), 10);
    for (int i = 0; i < 10; i++)
    {
      SOUL_TEST_ASSERT_EQ(test_variant.ref<ListTestObject>()[i], TestObject(i));
    }
    SOUL_TEST_ASSERT_TRUE(test_variant.has_value<ListTestObject>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<TestObject>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<u8>());
  }
}

TEST(TestVariantConstructor, TestCopyConstructor)
{
  {
    const auto test_variant_src = TrivialVariant::From(u16(20));
    const auto test_variant     = test_variant_src;
    SOUL_TEST_ASSERT_EQ(test_variant.ref<u16>(), 20);
    SOUL_TEST_ASSERT_TRUE(test_variant.has_value<u16>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<u8>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<TrivialObj>());
    SOUL_TEST_ASSERT_EQ(test_variant, test_variant_src);
  }

  {
    const auto test_variant_src = TrivialObj{.x = 30};
    auto test_variant_dst       = test_variant_src;
    const auto test_variant     = TrivialVariant::From(test_variant_dst);
    SOUL_TEST_ASSERT_EQ(test_variant.ref<TrivialObj>().x, 30);
    SOUL_TEST_ASSERT_EQ(test_variant.ref<TrivialObj>().y, 0);
    SOUL_TEST_ASSERT_TRUE(test_variant.has_value<TrivialObj>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<u8>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<u16>());
  }
}

TEST(TestVariantConstruction, TestClone)
{
  {
    const auto test_variant_src = UntrivialVariant::From(ListTestObject::From(
      std::views::iota(0, 10) | std::views::transform(
                                  [](int i)
                                  {
                                    return TestObject(i);
                                  })));
    const auto test_variant_dst = test_variant_src.clone();
    SOUL_TEST_ASSERT_EQ(test_variant_dst.ref<ListTestObject>().size(), 10);
    for (int i = 0; i < 10; i++)
    {
      SOUL_TEST_ASSERT_EQ(test_variant_dst.ref<ListTestObject>()[i], TestObject(i));
    }
    SOUL_TEST_ASSERT_TRUE(test_variant_dst.has_value<ListTestObject>());
    SOUL_TEST_ASSERT_FALSE(test_variant_dst.has_value<TestObject>());
    SOUL_TEST_ASSERT_FALSE(test_variant_dst.has_value<u8>());
  }

  {
    auto test_variant_src = MoveOnlyVariant::From(u8(3));
  }
}

TEST(TestVariantAssignmentOperator, TestCopyAssignment)
{
  {
    const auto test_variant_src = TrivialVariant::From(u16(20));
    auto test_variant_dst       = TrivialVariant::From(u16(40));
    test_variant_dst            = test_variant_src;
    SOUL_TEST_ASSERT_EQ(test_variant_dst.ref<u16>(), 20);
    SOUL_TEST_ASSERT_TRUE(test_variant_dst.has_value<u16>());
    SOUL_TEST_ASSERT_FALSE(test_variant_dst.has_value<u8>());
    SOUL_TEST_ASSERT_FALSE(test_variant_dst.has_value<TrivialObj>());
    SOUL_TEST_ASSERT_EQ(test_variant_dst, test_variant_src);
  }
}

TEST(TestVariantAssignmentOperator, TestMoveAssignment)
{
  {
    auto test_variant_src  = UntrivialVariant::From(ListTestObject::From(
      std::views::iota(0, 10) | std::views::transform(
                                  [](int i)
                                  {
                                    return TestObject(i);
                                  })));
    auto test_variant_dst  = UntrivialVariant::From(ListTestObject::From(
      std::views::iota(3, 10) | std::views::transform(
                                  [](int i)
                                  {
                                    return TestObject(i);
                                  })));
    auto test_variant_src2 = UntrivialVariant::From(u8(3));
    test_variant_dst       = std::move(test_variant_src);
    SOUL_TEST_ASSERT_EQ(test_variant_dst.ref<ListTestObject>().size(), 10);
    for (int i = 0; i < 10; i++)
    {
      SOUL_TEST_ASSERT_EQ(test_variant_dst.ref<ListTestObject>()[i], TestObject(i));
    }
    SOUL_TEST_ASSERT_TRUE(test_variant_dst.has_value<ListTestObject>());
    SOUL_TEST_ASSERT_FALSE(test_variant_dst.has_value<TestObject>());
    SOUL_TEST_ASSERT_FALSE(test_variant_dst.has_value<u8>());
  }
}

TEST(TestVariantCloneFrom, TestCloneFrom)
{
  {
    auto test_variant_src = UntrivialVariant::From(ListTestObject::From(
      std::views::iota(0, 10) | std::views::transform(
                                  [](int i)
                                  {
                                    return TestObject(i);
                                  })));
    auto test_variant_dst = UntrivialVariant::From(ListTestObject::From(
      std::views::iota(3, 10) | std::views::transform(
                                  [](int i)
                                  {
                                    return TestObject(i);
                                  })));
    test_variant_dst.clone_from(test_variant_src);
    SOUL_TEST_ASSERT_EQ(test_variant_dst.ref<ListTestObject>().size(), 10);
    for (int i = 0; i < 10; i++)
    {
      SOUL_TEST_ASSERT_EQ(test_variant_dst.ref<ListTestObject>()[i], TestObject(i));
    }
    SOUL_TEST_ASSERT_TRUE(test_variant_dst.has_value<ListTestObject>());
    SOUL_TEST_ASSERT_FALSE(test_variant_dst.has_value<TestObject>());
    SOUL_TEST_ASSERT_FALSE(test_variant_dst.has_value<u8>());
    SOUL_TEST_ASSERT_EQ(test_variant_dst, test_variant_src);
  }
}

TEST(TestVariantAssign, TestAssign)
{
  {
    auto test_variant = TrivialVariant::From(u16(40));
    test_variant.assign(u16(20));
    SOUL_TEST_ASSERT_EQ(test_variant.ref<u16>(), 20);
    SOUL_TEST_ASSERT_TRUE(test_variant.has_value<u16>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<u8>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<TrivialObj>());
  }

  {
    const auto trivial_obj = TrivialObj{.x = 30};
    auto test_variant      = TrivialVariant::From(TrivialObj{.x = 40, .y = 10});
    test_variant.assign(trivial_obj);
    SOUL_TEST_ASSERT_EQ(test_variant.ref<TrivialObj>().x, 30);
    SOUL_TEST_ASSERT_EQ(test_variant.ref<TrivialObj>().y, 0);
    SOUL_TEST_ASSERT_TRUE(test_variant.has_value<TrivialObj>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<u8>());
    SOUL_TEST_ASSERT_FALSE(test_variant.has_value<u16>());
  }

  {
    auto test_list_obj_src = ListTestObject::From(
      std::views::iota(0, 10) | std::views::transform(
                                  [](int i)
                                  {
                                    return TestObject(i);
                                  }));
    auto test_variant_dst = UntrivialVariant::From(ListTestObject::From(
      std::views::iota(3, 10) | std::views::transform(
                                  [](int i)
                                  {
                                    return TestObject(i);
                                  })));
    test_variant_dst.assign(std::move(test_list_obj_src));
    SOUL_TEST_ASSERT_EQ(test_variant_dst.ref<ListTestObject>().size(), 10);
    for (int i = 0; i < 10; i++)
    {
      SOUL_TEST_ASSERT_EQ(test_variant_dst.ref<ListTestObject>()[i], TestObject(i));
    }
    SOUL_TEST_ASSERT_TRUE(test_variant_dst.has_value<ListTestObject>());
    SOUL_TEST_ASSERT_FALSE(test_variant_dst.has_value<TestObject>());
    SOUL_TEST_ASSERT_FALSE(test_variant_dst.has_value<u8>());
  }
}

TEST(TestVariantVisit, TestVisit)
{
  {
    enum class TrivialKind
    {
      UINT16,
      UINT8,
      TRIVIAL_OBJ
    };

    struct VisitResult
    {
      u16 val;
      TrivialKind kind;
      auto operator==(const VisitResult& other) const -> bool = default;
    };

    int x                  = 0;
    const auto visitor_set = soul::VisitorSet{
      [&x](u16 val)
      {
        x += val;
        return VisitResult{val, TrivialKind::UINT16};
      },
      [&x](u8 val)
      {
        x += val;
        return VisitResult{val, TrivialKind::UINT8};
      },
      [&x](TrivialObj obj)
      {
        x += obj.x;
        return VisitResult{obj.y, TrivialKind::TRIVIAL_OBJ};
      },
    };
    const auto expected_visit_result1 = VisitResult{20, TrivialKind::UINT16};
    SOUL_TEST_ASSERT_EQ(TrivialVariant::From(u16(20)).visit(visitor_set), expected_visit_result1);
    SOUL_TEST_ASSERT_EQ(x, 20);
    auto test_variant2                = TrivialVariant::From(TrivialObj{30, 15});
    const auto expected_visit_result2 = VisitResult{15, TrivialKind::TRIVIAL_OBJ};
    SOUL_TEST_ASSERT_EQ(test_variant2.visit(visitor_set), expected_visit_result2);
    SOUL_TEST_ASSERT_EQ(x, 50);
  }

  {
    enum class UntrivialKind
    {
      LIST_TEST_OBJECT,
      TEST_OBJECT,
      UINT8
    };

    struct VisitResult
    {
      ListTestObject val;
      UntrivialKind kind;
      auto operator==(const VisitResult& other) const -> bool = default;
    };

    usize x                = 0;
    const auto visitor_set = soul::VisitorSet{
      [&x](ListTestObject&& val)
      {
        x += val.size();
        return VisitResult{std::move(val), UntrivialKind::LIST_TEST_OBJECT};
      },
      [&x](TestObject&& val)
      {
        x += val.x;
        return VisitResult{ListTestObject::WithCapacity(5), UntrivialKind::TEST_OBJECT};
      },
      [&x](u8 val)
      {
        x += val;
        return VisitResult{ListTestObject::WithSize(10), UntrivialKind::UINT8};
      },
    };
    auto test_list_obj = ListTestObject::From(
      std::views::iota(3, 6) | std::views::transform(
                                 [](int val)
                                 {
                                   return TestObject(val);
                                 }));
    auto test_list_obj_copy = test_list_obj.clone();
    const auto visit_result = UntrivialVariant::From(std::move(test_list_obj)).visit(visitor_set);
    SOUL_TEST_ASSERT_EQ(visit_result.val, test_list_obj_copy);
    SOUL_TEST_ASSERT_EQ(x, 3);
    SOUL_TEST_ASSERT_EQ(visit_result.kind, UntrivialKind::LIST_TEST_OBJECT);
  }
}

TEST(TestVariantUnwrap, TestUnwrap)
{
  {
    auto val = TrivialVariant::From(u16(40)).unwrap<u16>();
    SOUL_TEST_ASSERT_EQ(val, 40);
  }
  {

    const auto unwrap_result =
      UntrivialVariant::From(ListTestObject::From(
                               std::views::iota(0, 10) | std::views::transform(
                                                           [](int i)
                                                           {
                                                             return TestObject(i);
                                                           })))
        .unwrap<ListTestObject>();

    for (int i = 0; i < 10; i++)
    {
      SOUL_TEST_ASSERT_EQ(unwrap_result[i], TestObject(i));
    }
  }
}

TEST(TestVariantSwap, TestSwap)
{
  {
    auto test_variant1 = UntrivialVariant::From(ListTestObject::From(
      std::views::iota(0, 10) | std::views::transform(
                                  [](int i)
                                  {
                                    return TestObject(i);
                                  })));
    auto test_variant2 = UntrivialVariant::From(u8(10));
    swap(test_variant1, test_variant2);
    SOUL_TEST_ASSERT_EQ(test_variant1.ref<u8>(), 10);
    for (int i = 0; i < 10; i++)
    {
      SOUL_TEST_ASSERT_EQ(test_variant2.ref<ListTestObject>()[i], TestObject(i));
    }
  }
}

TEST(TestVariantHash, TestHash)
{
  static_assert(typeset<TrivialVariant>);
  static_assert(std::is_trivially_copyable_v<TrivialVariant>);
  SOUL_TEST_RUN(test_hash_implementation(Array{
    TrivialVariant::From(u16(20)),
    TrivialVariant::From(u16(2)),
    TrivialVariant::From(u8(2)),
    TrivialVariant::From(u8(20)),
    TrivialVariant::From(TrivialObj{3, 4}),
    TrivialVariant::From(TrivialObj{2, 0}),
  }));
}
