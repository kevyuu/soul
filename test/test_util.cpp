#include "core/util.h"

#include "util.h"

TEST(TestCoreUtil, TestNextPowerOfTwo)
{
  SOUL_TEST_ASSERT_EQ(soul::util::next_power_of_two(0), 1);
  SOUL_TEST_ASSERT_EQ(soul::util::next_power_of_two(1), 2);
  SOUL_TEST_ASSERT_EQ(soul::util::next_power_of_two(2), 4);
  SOUL_TEST_ASSERT_EQ(soul::util::next_power_of_two(3), 4);
  SOUL_TEST_ASSERT_EQ(soul::util::next_power_of_two((1 << 8) - 1), (1 << 8));
  SOUL_TEST_ASSERT_EQ(soul::util::next_power_of_two((1LLU << 60) + 2), (1LLU << 61));
}

TEST(TestCoreUtil, TestGetFirstOneBitPos)
{
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(u8{0b1000'0000}).some_ref(), 7);
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(u8{0b0100'0001}).some_ref(), 0);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_first_one_bit_pos(u8{0}).is_some());

  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(u16{0b1000'0000'0000'0000}).some_ref(), 15);
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(u16{0b0100'0100'0100'0001}).some_ref(), 0);
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(static_cast<u16>(~u16{0})).some_ref(), 0);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_first_one_bit_pos(u16{0}).is_some());

  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(u32{1}).some_ref(), 0);
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(u32{0x80000000}).some_ref(), 31);
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(u32{0x40000000}).some_ref(), 30);
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(~u32{0}).some_ref(), 0);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_first_one_bit_pos(u32{0}).is_some());

  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(u64{1}).some_ref(), 0);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_first_one_bit_pos(u64{0}).is_some());
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(~u64{0}).some_ref(), 0);
  SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(u64{0x40000000}).some_ref(), 30);
  SOUL_TEST_ASSERT_EQ(
    soul::util::get_first_one_bit_pos(u64{18446744069414584320llu}).some_ref(), 32);
}

TEST(TestCoreUtil, TestGetLastOneBitPos)
{
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(u8{0b1000'0000}).some_ref(), 7);
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(u8{0b0100'0001}).some_ref(), 6);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_last_one_bit_pos(u8{0}).is_some());

  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(u16{0b1000'0000'0000'0000}).some_ref(), 15);
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(u16{0b0100'0100'0100'0001}).some_ref(), 14);
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(static_cast<u16>(~u16{0})).some_ref(), 15);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_last_one_bit_pos(u16{0}).is_some());

  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(u32{1}).some_ref(), 0);
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(u32{0x80000000}).some_ref(), 31);
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(u32{0x400F0200}).some_ref(), 30);
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(~u32{0}).some_ref(), 31);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_last_one_bit_pos(u32{0}).is_some());

  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(u64{1}).some_ref(), 0);
  SOUL_TEST_ASSERT_FALSE(soul::util::get_last_one_bit_pos(u64{0}).is_some());
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(~u64{0}).some_ref(), 63);
  SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(u64{0x4F000200}).some_ref(), 30);
}

TEST(TestCoreUtil, TestGetOneBitCount)
{
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(u8{0b1000'0000}), 1);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(u8{0b0100'0001}), 2);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(u8{0}), 0);

  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(u16{0b1000'0000'0000'0000}), 1);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(u16{0b0100'0100'0100'0001}), 4);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(static_cast<u16>(~u16{0})), 16);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(u16{0}), 0);

  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(u32{1}), 1);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(u32{0x80000000}), 1);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(u32{0x400F0200}), 6);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(~u32{0}), 32);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(u32{0}), 0);

  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(u64{1}), 1);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(u64{0}), 0);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(~u64{0}), 64);
  SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(u64{0x4F000200}), 6);

  {
    constexpr auto test_count = soul::util::get_one_bit_count(u16{0b0100'0100'0100'0001});
    SOUL_TEST_ASSERT_EQ(test_count, 4);
  }
}

TEST(TestCoreUtil, TestForEachOneBitPos)
{
  auto test_for_each_one_bit_pos =
    []<std::integral T>(T val, const std::vector<u32>& expected_value) {
      std::vector<u32> bit_pos;
      soul::util::for_each_one_bit_pos(
        val, [&bit_pos](const usize position) { bit_pos.push_back(position); });

      SOUL_TEST_ASSERT_TRUE(std::ranges::equal(bit_pos, expected_value));
    };

  SOUL_TEST_RUN(test_for_each_one_bit_pos(u8{0b1000'0000}, std::vector<u32>({7})));
  SOUL_TEST_RUN(test_for_each_one_bit_pos(u8{0}, std::vector<u32>()));

  std::vector<u32> expected_result_all_set(16);
  std::iota(std::begin(expected_result_all_set), std::end(expected_result_all_set), 0);
  SOUL_TEST_RUN(test_for_each_one_bit_pos(static_cast<u16>(~u16{0}), expected_result_all_set));

  SOUL_TEST_RUN(
    test_for_each_one_bit_pos(u64{0x4F000200}, std::vector<u32>({9, 24, 25, 26, 27, 30})));
}

TEST(TestCoreUtil, TestDigitCount)
{
  SOUL_TEST_ASSERT_EQ(soul::util::digit_count(100), 3);
  SOUL_TEST_ASSERT_EQ(soul::util::digit_count(100, 10), 3);
  SOUL_TEST_ASSERT_EQ(soul::util::digit_count(3), 1);
  SOUL_TEST_ASSERT_EQ(soul::util::digit_count(0), 1);
  SOUL_TEST_ASSERT_EQ(soul::util::digit_count(0xF3, 16), 2);
  SOUL_TEST_ASSERT_EQ(soul::util::digit_count(0x0, 16), 1);
}

TEST(TestCoreUtil, TestUnalignedLoad)
{
  {
    u32 test = 623413;
    SOUL_TEST_ASSERT_EQ(
      soul::util::unaligned_load32(reinterpret_cast<const soul::byte*>(&test)), test);

    const auto* test_bytes = reinterpret_cast<const soul::byte*>(&test);
    const soul::byte test_unaligned_bytes[5] = {
      0,
      test_bytes[0],
      test_bytes[1],
      test_bytes[2],
      test_bytes[3],
    };
    SOUL_TEST_ASSERT_EQ(
      soul::util::unaligned_load32(reinterpret_cast<const soul::byte*>(&test)), test);
  }

  {
    u64 test = 647384999425089;
    SOUL_TEST_ASSERT_EQ(
      soul::util::unaligned_load64(reinterpret_cast<const soul::byte*>(&test)), test);

    const auto* test_bytes = reinterpret_cast<const soul::byte*>(&test);
    const soul::byte test_unaligned_bytes[9] = {
      0,
      test_bytes[0],
      test_bytes[1],
      test_bytes[2],
      test_bytes[3],
      test_bytes[4],
      test_bytes[5],
      test_bytes[6],
      test_bytes[7],
    };
    SOUL_TEST_ASSERT_EQ(soul::util::unaligned_load64(test_unaligned_bytes + 1), test);
  }
}
