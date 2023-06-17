#pragma once

#include <algorithm>
#include <cinttypes>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include <xutility>
#include <gtest/gtest.h>

#include "core/type_traits.h"
#include "core/vector.h"
#include "core/views.h"
#include "memory/allocator.h"

constexpr uint32_t K_MAGIC_VALUE = 0x01f1cbe8;

struct TestObject {
  int x;            // Value for the TestObject.
  bool throwOnCopy; // Throw an exception of this object is copied, moved, or assigned to another.
  int64_t id;
  // Unique id for each object, equal to its creation number. This value is not coped from other
  // TestObjects during any operations, including moves.
  uint32_t magicValue;
  // Used to verify that an instance is valid and that it is not corrupted. It should always be
  // kMagicValue.
  static int64_t sTOCount;            // Count of all current existing TestObjects.
  static int64_t sTOCtorCount;        // Count of times any ctor was called.
  static int64_t sTODtorCount;        // Count of times dtor was called.
  static int64_t sTODefaultCtorCount; // Count of times the default ctor was called.
  static int64_t sTOArgCtorCount;     // Count of times the x0,x1,x2 ctor was called.
  static int64_t sTOCopyCtorCount;    // Count of times copy ctor was called.
  static int64_t sTOMoveCtorCount;    // Count of times move ctor was called.
  static int64_t sTOCopyAssignCount;  // Count of times copy assignment was called.
  static int64_t sTOMoveAssignCount;  // Count of times move assignment was called.
  static int sMagicErrorCount;        // Number of magic number mismatch errors.

  explicit TestObject(int x = 0, bool bThrowOnCopy = false)
      : x(x), throwOnCopy(bThrowOnCopy), magicValue(K_MAGIC_VALUE)
  {
    ++sTOCount;
    ++sTOCtorCount;
    ++sTODefaultCtorCount;
    id = sTOCtorCount;
  }

  // This constructor exists for the purpose of testing variadiac template arguments, such as with
  // the emplace container functions.
  TestObject(int x0, int x1, int x2, const bool throw_on_copy = false)
      : x(x0 + x1 + x2), throwOnCopy(throw_on_copy), magicValue(K_MAGIC_VALUE)
  {
    ++sTOCount;
    ++sTOCtorCount;
    ++sTOArgCtorCount;
    id = sTOCtorCount;
  }

  // Due to the nature of TestObject, there isn't much special for us to
  // do in our move constructor. A move constructor swaps its contents with
  // the other object, whhich is often a default-constructed object.
  // ReSharper disable once CppSpecialFunctionWithoutNoexceptSpecification
  TestObject(TestObject&& test_object)
      : x(test_object.x), throwOnCopy(test_object.throwOnCopy), magicValue(test_object.magicValue)
  {
    ++sTOCount;
    ++sTOCtorCount;
    ++sTOMoveCtorCount;
    id = sTOCtorCount; // testObject keeps its mId, and we assign ours anew.
    test_object.x =
      0; // We are swapping our contents with the TestObject, so give it our "previous" value.
    if (throwOnCopy) {
      std::cout << "Disallowd TestObject copy" << std::endl;
      SOUL_PANIC("Disallowed TestObject copy");
    }
  }

  // ReSharper disable once CppSpecialFunctionWithoutNoexceptSpecification
  auto operator=(TestObject&& testObject) -> TestObject&
  {
    ++sTOMoveAssignCount;

    if (&testObject != this) {
      std::swap(x, testObject.x);
      // Leave mId alone.
      std::swap(magicValue, testObject.magicValue);
      std::swap(throwOnCopy, testObject.throwOnCopy);

      if (throwOnCopy) {
        SOUL_PANIC("Disallowed TestObject copy");
      }
    }
    return *this;
  }

  ~TestObject()
  {
    if (magicValue != K_MAGIC_VALUE) {
      ++sMagicErrorCount;
    }
    magicValue = 0;
    --sTOCount;
    ++sTODtorCount;
  }

  static auto reset() -> void
  {
    sTOCount = 0;
    sTOCtorCount = 0;
    sTODtorCount = 0;
    sTODefaultCtorCount = 0;
    sTOArgCtorCount = 0;
    sTOCopyCtorCount = 0;
    sTOMoveCtorCount = 0;
    sTOCopyAssignCount = 0;
    sTOMoveAssignCount = 0;
    sMagicErrorCount = 0;
  }

  static auto is_clear() -> bool
  // Returns true if there are no existing TestObjects and the sanity checks related to that test
  // OK.

  {
    return (sTOCount == 0) && (sTODtorCount == sTOCtorCount) && (sMagicErrorCount == 0);
  }

protected:
  TestObject(const TestObject& test_object)
      : x(test_object.x), throwOnCopy(test_object.throwOnCopy), magicValue(test_object.magicValue)
  {
    ++sTOCount;
    ++sTOCtorCount;
    ++sTOCopyCtorCount;
    id = sTOCtorCount;
    if (throwOnCopy) {
      SOUL_PANIC("Disallowed TestObject copy");
    }
  }

  auto operator=(const TestObject& testObject) -> TestObject&
  {
    ++sTOCopyAssignCount;

    if (&testObject != this) {
      x = testObject.x;
      // Leave mId alone.
      magicValue = testObject.magicValue;
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

inline auto operator==(const TestObject& t1, const TestObject& t2) -> bool { return t1.x == t2.x; }

inline auto operator<(const TestObject& t1, const TestObject& t2) -> bool { return t1.x < t2.x; }

using ListTestObject = soul::Vector<TestObject>;
static_assert(soul::ts_clone<ListTestObject>);

class TestAllocator : public soul::memory::Allocator
{
public:
  explicit TestAllocator(const char* name = "Test Malloc Allocator")
      : Allocator(name), allocCount(0), freeCount(0), allocVolume(0)
  {
  }

  auto try_allocate(soul_size size, soul_size alignment, const char* tag)
    -> soul::memory::Allocation override;
  auto get_allocation_size(void* addr) const -> soul_size override;
  auto deallocate(void* addr) -> void override;

  auto reset() -> void override {}

  static auto reset_all() -> void
  {
    allocCountAll = 0;
    freeCountAll = 0;
    allocVolumeAll = 0;
    lastAllocation = nullptr;
  }

public:
  int allocCount;
  int freeCount;
  size_t allocVolume;

  static int allocCountAll;
  static int freeCountAll;
  static size_t allocVolumeAll;
  static void* lastAllocation;
};

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

template <typename T>
using Sequence = std::vector<T>;

template <typename T>
static auto generate_random_sequence(const soul_size size) -> Sequence<T>
{
  std::random_device random_device;
  std::mt19937 random_engine(random_device());
  std::uniform_int_distribution<int> dist(1, 100);

  Sequence<T> result(size);
  if constexpr (std::same_as<T, ListTestObject>) {
    std::generate(result.begin(), result.end(), [&]() {
      return ListTestObject::generate_n(soul::clone_fn(TestObject(dist(random_engine))), 10);
    });
  } else {
    std::generate(result.begin(), result.end(), [&]() { return T(dist(random_engine)); });
  }
  return result;
}

template <typename T>
static auto generate_sequence(const soul_size size, const T& val) -> Sequence<T>
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
