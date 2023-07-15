#include "core/util.h"

#include "util.h"

TEST(TestCoreUtil, TestGetFirstOneBitPos)
{
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(ui8{0b1000'0000}).value(), 7);
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(ui8{0b0100'0001}).value(), 0);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_first_one_bit_pos(ui8{0}));

  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(ui16{0b1000'0000'0000'0000}).value(), 15);
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(ui16{0b0100'0100'0100'0001}).value(), 0);
  SOUL_TEST_ASSERT_EQ(
    soul::util::get_first_one_bit_pos(static_cast<ui16>(~ui16{0})).value(), 0);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_first_one_bit_pos(ui16{0}));

  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(ui32{1}).value(), 0);
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(ui32{0x80000000}).value(), 31);
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(ui32{0x40000000}).value(), 30);
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(~ui32{0}).value(), 0);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_first_one_bit_pos(ui32{0}));

  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(ui64{1}).value(), 0);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_first_one_bit_pos(ui64{0}));
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(~ui64{0}).value(), 0);
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(ui64{0x40000000}).value(), 30);
  SOUL_TEST_ASSERT_EQ(
    soul::util::get_first_one_bit_pos(ui64{18446744069414584320llu}).value(), 32);
}

TEST(TestCoreUtil, TestGetLastOneBitPos)
{
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(ui8{0b1000'0000}).value(), 7);
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(ui8{0b0100'0001}).value(), 6);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_last_one_bit_pos(ui8{0}));

  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(ui16{0b1000'0000'0000'0000}).value(), 15);
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(ui16{0b0100'0100'0100'0001}).value(), 14);
  SOUL_TEST_ASSERT_EQ(
    soul::util::get_last_one_bit_pos(static_cast<ui16>(~ui16{0})).value(), 15);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_last_one_bit_pos(ui16{0}));

  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(ui32{1}).value(), 0);
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(ui32{0x80000000}).value(), 31);
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(ui32{0x400F0200}).value(), 30);
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(~ui32{0}).value(), 31);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_last_one_bit_pos(ui32{0}));

  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(ui64{1}).value(), 0);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_last_one_bit_pos(ui64{0}));
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(~ui64{0}).value(), 63);
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(ui64{0x4F000200}).value(), 30);
}

TEST(TestCoreUtil, TestGetOneBitCount)
{
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(ui8{0b1000'0000}), 1);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(ui8{0b0100'0001}), 2);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(ui8{0}), 0);

  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(ui16{0b1000'0000'0000'0000}), 1);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(ui16{0b0100'0100'0100'0001}), 4);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(static_cast<ui16>(~ui16{0})), 16);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(ui16{0}), 0);

  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(ui32{1}), 1);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(ui32{0x80000000}), 1);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(ui32{0x400F0200}), 6);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(~ui32{0}), 32);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(ui32{0}), 0);

  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(ui64{1}), 1);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(ui64{0}), 0);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(~ui64{0}), 64);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(ui64{0x4F000200}), 6);

  {
    constexpr auto test_count = soul::util::get_one_bit_count(ui16{0b0100'0100'0100'0001});
    SOUL_TEST_ASSERT_EQ(test_count, 4);
  }
}

TEST(TestCoreUtil, TestForEachOneBitPos)
{
  auto test_for_each_one_bit_pos =
    []<std::integral T>(T val, const std::vector<ui32>& expected_value) {
      std::vector<ui32> bit_pos;
      soul::util::for_each_one_bit_pos(
        val, [&bit_pos](const usize position) { bit_pos.push_back(position); });

      SOUL_TEST_ASSERT_TRUE(std::ranges::equal(bit_pos, expected_value));
    };

  SOUL_TEST_RUN(test_for_each_one_bit_pos(ui8{0b1000'0000}, std::vector<ui32>({7})));
  SOUL_TEST_RUN(test_for_each_one_bit_pos(ui8{0}, std::vector<ui32>()));

  std::vector<ui32> expected_result_all_set(16);
  std::iota(std::begin(expected_result_all_set), std::end(expected_result_all_set), 0);
  SOUL_TEST_RUN(
    test_for_each_one_bit_pos(static_cast<ui16>(~ui16{0}), expected_result_all_set));

  SOUL_TEST_RUN(
    test_for_each_one_bit_pos(ui64{0x4F000200}, std::vector<ui32>({9, 24, 25, 26, 27, 30})));
}
