#pragma once

#include <algorithm>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "core/log.h"
#include "core/panic_format.h"
#include "core/type_traits.h"
#include "core/vector.h"
#include "core/views.h"
#include "memory/allocator.h"

#include "builtins.h"

static const char* const DEFAULT_SOUL_TEST_MESSAGE = "---";
static std::vector<std::string> soul_test_messages;

inline static auto get_soul_test_message() -> std::string
{
  if (soul_test_messages.empty()) {
    return "-----";
  }
  std::string str = std::accumulate(
    soul_test_messages.begin() + 1,
    soul_test_messages.end(),
    soul_test_messages[0],
    [](std::string x, std::string y) { return x + "::" + y; });
  if (str.empty()) {
    return DEFAULT_SOUL_TEST_MESSAGE;
  }
  return str;
}

struct SoulTestMessageScope {
  explicit SoulTestMessageScope(const char* message) { soul_test_messages.push_back(message); }

  SoulTestMessageScope(const SoulTestMessageScope&) = delete;
  SoulTestMessageScope(SoulTestMessageScope&&) = delete;
  auto operator=(const SoulTestMessageScope&) -> SoulTestMessageScope& = delete;
  auto operator=(SoulTestMessageScope&&) -> SoulTestMessageScope& = delete;

  ~SoulTestMessageScope() { soul_test_messages.pop_back(); }
};

#define SOUL_SINGLE_ARG(...) __VA_ARGS__
#define SOUL_TEST_MESSAGE(message) SoulTestMessageScope test_message_scope(message)
#define SOUL_TEST_RUN(expr)                                                                        \
  do {                                                                                             \
    SOUL_TEST_MESSAGE(#expr);                                                                      \
    expr;                                                                                          \
  } while (0)
#define SOUL_TEST_ASSERT_EQ(expr1, expr2)                                                          \
  ASSERT_EQ(expr1, expr2) << "Case : " << get_soul_test_message()
#define SOUL_TEST_ASSERT_STREQ(expr1, expr2)                                                       \
  ASSERT_STREQ(expr1, expr2) << "Case : " << get_soul_test_message()
#define SOUL_TEST_ASSERT_NE(expr1, expr2)                                                          \
  ASSERT_NE(expr1, expr2) << "Case : " << get_soul_test_message()
#define SOUL_TEST_ASSERT_GE(expr1, expr2)                                                          \
  ASSERT_GE(expr1, expr2) << "Case : " << get_soul_test_message()
#define SOUL_TEST_ASSERT_GT(expr1, expr2)                                                          \
  ASSERT_GT(expr1, expr2) << "Case : " << get_soul_test_message()
#define SOUL_TEST_ASSERT_LE(expr1, expr2)                                                          \
  ASSERT_LE(expr1, expr2) << "Case : " << get_soul_test_message()
#define SOUL_TEST_ASSERT_LT(expr1, expr2)                                                          \
  ASSERT_LT(expr1, expr2) << "Case : " << get_soul_test_message()
#define SOUL_TEST_ASSERT_TRUE(expr) ASSERT_TRUE(expr) << "Case : " << get_soul_test_message()
#define SOUL_TEST_ASSERT_FALSE(expr) ASSERT_FALSE(expr) << "Case : " << get_soul_test_message()

struct TestObject {
  int x;          // Value for the TestObject.
  b8 throwOnCopy; // Throw an exception of this object is copied, moved, or assigned to another.
  int64_t id;
  // Unique id for each object, equal to its creation number. This value is not coped from other
  // TestObjects during any operations, including moves.
  uint32_t state_value;

  static constexpr u32 K_CONSTRUCTED_STATE = 0x01f1cbe8;
  static constexpr u32 K_MOVED_FROM_STATE = 3;
  static constexpr u32 K_DESTRUCTED_STATE = 0;

  static auto StateStr(u32 state) -> const char*
  {
    if (state == K_CONSTRUCTED_STATE) {
      return "constructed_state";
    } else if (state == K_MOVED_FROM_STATE) {
      return "moved_from_state";
    } else if (state == K_DESTRUCTED_STATE) {
      return "destructed_state";
    }
    return "invalid_state";
  }

  struct Diagnostic {
    i64 obj_count = 0;          // Count of all current existing TestObjects.
    i64 ctor_count = 0;         // Count of times any ctor was called.
    i64 dtor_count = 0;         // Count of times dtor was called.
    i64 default_ctor_count = 0; // Count of times the default ctor was called.
    i64 arg_ctor_count = 0;     // Count of times the x0,x1,x2 ctor was called.
    i64 copy_ctor_count = 0;    // Count of times copy ctor was called.
    i64 move_ctor_count = 0;    // Count of times move ctor was called.
    i64 copy_assign_count = 0;  // Count of times copy assignment was called.
    i64 move_assign_count = 0;  // Count of times move assignment was called.
    i32 magic_error_count = 0;  // Number of magic number mismatch errors.

    [[nodiscard]]
    auto
    operator-(const Diagnostic& other) const -> Diagnostic
    {
      return {
        obj_count - other.obj_count,
        ctor_count - other.ctor_count,
        dtor_count - other.dtor_count,
        default_ctor_count - other.default_ctor_count,
        arg_ctor_count - other.arg_ctor_count,
        copy_ctor_count - other.copy_ctor_count,
        move_ctor_count - other.move_ctor_count,
        copy_assign_count - other.copy_assign_count,
        move_assign_count - other.move_assign_count,
        magic_error_count - other.magic_error_count};
    }
  };

  static Diagnostic s_diagnostic;

  explicit TestObject(int x = 0, b8 bThrowOnCopy = false)
      : x(x), throwOnCopy(bThrowOnCopy), state_value(K_CONSTRUCTED_STATE)
  {
    ++s_diagnostic.obj_count;
    ++s_diagnostic.ctor_count;
    ++s_diagnostic.default_ctor_count;
    id = s_diagnostic.ctor_count;
  }

  // This constructor exists for the purpose of testing variadiac template arguments, such as with
  // the emplace container functions.
  TestObject(int x0, int x1, int x2, const b8 throw_on_copy = false)
      : x(x0 + x1 + x2), throwOnCopy(throw_on_copy), state_value(K_CONSTRUCTED_STATE)
  {
    ++s_diagnostic.obj_count;
    ++s_diagnostic.ctor_count;
    ++s_diagnostic.arg_ctor_count;
    id = s_diagnostic.ctor_count;
  }

  // Due to the nature of TestObject, there isn't much special for us to
  // do in our move constructor. A move constructor swaps its contents with
  // the other object, whhich is often a default-constructed object.
  // ReSharper disable once CppSpecialFunctionWithoutNoexceptSpecification
  TestObject(TestObject&& test_object) : x(test_object.x), throwOnCopy(test_object.throwOnCopy)
  {
    SOUL_ASSERT_FORMAT(
      0,
      test_object.state_value == K_CONSTRUCTED_STATE,
      "Moving {} object with x : {}",
      StateStr(test_object.state_value),
      x);
    state_value = std::exchange(test_object.state_value, K_MOVED_FROM_STATE);
    ++s_diagnostic.ctor_count;
    ++s_diagnostic.move_ctor_count;
    id = s_diagnostic.ctor_count; // testObject keeps its mId, and we assign ours anew.
    if (throwOnCopy) {
      SOUL_PANIC("Disallowed TestObject copy");
    }
  }

  // ReSharper disable once CppSpecialFunctionWithoutNoexceptSpecification
  auto operator=(TestObject&& testObject) -> TestObject&
  {
    ++s_diagnostic.move_assign_count;

    if (&testObject != this) {
      std::swap(x, testObject.x);
      // Leave id alone.
      std::swap(state_value, testObject.state_value);
      std::swap(throwOnCopy, testObject.throwOnCopy);

      if (throwOnCopy) {
        SOUL_PANIC("Disallowed TestObject copy");
      }
    }
    return *this;
  }

  ~TestObject()
  {
    if (state_value != K_CONSTRUCTED_STATE && state_value != K_MOVED_FROM_STATE) {
      SOUL_PANIC_FORMAT("state value is wrong. state_value : {}", StateStr(state_value));
      ++s_diagnostic.magic_error_count;
    }
    if (state_value == K_CONSTRUCTED_STATE) {
      s_diagnostic.obj_count--;
    }
    state_value = 0;
    ++s_diagnostic.dtor_count;
  }

  static auto reset() -> void
  {
    s_diagnostic.obj_count = 0;
    s_diagnostic.ctor_count = 0;
    s_diagnostic.dtor_count = 0;
    s_diagnostic.default_ctor_count = 0;
    s_diagnostic.arg_ctor_count = 0;
    s_diagnostic.copy_ctor_count = 0;
    s_diagnostic.move_ctor_count = 0;
    s_diagnostic.copy_assign_count = 0;
    s_diagnostic.move_assign_count = 0;
    s_diagnostic.magic_error_count = 0;
  }

  static auto IsClear() -> b8
  // Returns true if there are no existing TestObjects and the sanity checks related to that test
  // OK.
  {
    return (s_diagnostic.obj_count == 0) && (s_diagnostic.dtor_count == s_diagnostic.ctor_count) &&
           (s_diagnostic.magic_error_count == 0);
  }

protected:
  TestObject(const TestObject& test_object)
      : x(test_object.x), throwOnCopy(test_object.throwOnCopy), state_value(test_object.state_value)
  {
    ++s_diagnostic.obj_count;
    ++s_diagnostic.ctor_count;
    ++s_diagnostic.copy_ctor_count;
    id = s_diagnostic.ctor_count;
    if (throwOnCopy) {
      SOUL_PANIC("Disallowed TestObject copy");
    }
  }

  auto operator=(const TestObject& testObject) -> TestObject&
  {
    ++s_diagnostic.copy_assign_count;

    if (&testObject != this) {
      x = testObject.x;
      // Leave mId alone.
      state_value = testObject.state_value;
      throwOnCopy = testObject.throwOnCopy;
      if (throwOnCopy) {
        SOUL_PANIC("Disallowed TestObject copy");
      }
    }
    return *this;
  }

public:
  [[nodiscard]]
  auto clone() const -> TestObject
  {
    return TestObject(*this);
  }

  void clone_from(const TestObject& test_obj) { *this = test_obj; }

  friend class std::vector<TestObject>;
};
static_assert(soul::ts_clone<TestObject>);

inline auto operator==(const TestObject& t1, const TestObject& t2) -> b8 { return t1.x == t2.x; }

inline auto operator<(const TestObject& t1, const TestObject& t2) -> b8 { return t1.x < t2.x; }

using ListTestObject = soul::Vector<TestObject>;
static_assert(soul::ts_clone<ListTestObject>);

class TestAllocator : public soul::memory::Allocator
{
public:
  explicit TestAllocator(const char* name = "Test Malloc Allocator") : Allocator(name) {}

  auto try_allocate(usize size, usize alignment, const char* tag)
    -> soul::memory::Allocation override;
  auto get_allocation_size(void* addr) const -> usize override;
  auto deallocate(void* addr) -> void override;

  auto reset() -> void override {}

  static auto reset_all() -> void
  {
    allocCountAll = 0;
    freeCountAll = 0;
    allocVolumeAll = 0;
    lastAllocation = nullptr;
  }

  TestAllocator(const TestAllocator&) = delete;
  TestAllocator(TestAllocator&&) noexcept = delete;
  auto operator=(const TestAllocator&) = delete;
  auto operator=(TestAllocator&&) noexcept = delete;
  ~TestAllocator() override
  {
    SOUL_ASSERT_FORMAT(0, allocVolume == 0, "Alloc Volume : {}", allocVolume);
    SOUL_ASSERT_FORMAT(
      0, allocCount == freeCount, "Alloc Count : {}, Free Count : {}", allocCount, freeCount);
  }

  int allocCount{};
  int freeCount{};
  size_t allocVolume{};

  static int allocCountAll;
  static int freeCountAll;
  static size_t allocVolumeAll;
  static void* lastAllocation;
};

template <typename T>
using Sequence = std::vector<T>;

template <typename T>
static auto generate_random_sequence(const usize size) -> Sequence<T>
{
  std::random_device random_device;
  std::mt19937 random_engine(random_device());
  std::uniform_int_distribution<int> dist(1, 100);

  Sequence<T> result(size);
  if constexpr (std::same_as<T, ListTestObject>) {
    std::generate(result.begin(), result.end(), [&]() {
      return ListTestObject::GenerateN(soul::clone_fn(TestObject(dist(random_engine))), 10);
    });
  } else {
    std::generate(result.begin(), result.end(), [&]() { return T(dist(random_engine)); });
  }
  return result;
}

template <typename T>
static auto generate_sequence(const usize size, const T& val) -> Sequence<T>
{
  Sequence<T> sequence(size);
  std::generate(sequence.begin(), sequence.end(), soul::duplicate_fn(val));
  return sequence;
}

template <typename T, std::input_iterator Iterator>
static auto generate_sequence(Iterator first, Iterator last) -> Sequence<T>
{
  auto range = std::ranges::subrange(first, last) | soul::views::duplicate<T>();
  Sequence<T> sequence(range.begin(), range.end());
  return sequence;
}

template <typename T>
static auto generate_sequence(const Sequence<T>& sequence1, const Sequence<T>& sequence2)
  -> Sequence<T>
{
  auto range1 = sequence1 | soul::views::duplicate<T>();
  Sequence<T> sequence(range1.begin(), range1.end());
  auto range2 = sequence2 | soul::views::duplicate<T>();
  sequence.insert(sequence.end(), range2.begin(), range2.end());
  return sequence;
}

struct ProgramExitCheck {
  ProgramExitCheck() = default;
  ProgramExitCheck(const ProgramExitCheck&) = default;
  ProgramExitCheck(ProgramExitCheck&&) = delete;
  ProgramExitCheck& operator=(const ProgramExitCheck&) = default;
  ProgramExitCheck& operator=(ProgramExitCheck&&) = delete;
  ~ProgramExitCheck()
  {
    SOUL_ASSERT_FORMAT(
      0,
      TestObject::IsClear(),
      "Test Object not being cleaned up properly!\n"
      "obj_count : {},\n"
      "ctor_count : {},\n"
      "move_ctor_count : {}\n"
      "dtor_count : {},\n"
      "magic_error_count : {}\n",
      TestObject::s_diagnostic.obj_count,
      TestObject::s_diagnostic.ctor_count,
      TestObject::s_diagnostic.move_ctor_count,
      TestObject::s_diagnostic.dtor_count,
      TestObject::s_diagnostic.magic_error_count);
  }
};
const auto test_object_exist_assert = ProgramExitCheck();
