#include "core/util.h"

#include "util.h"

TEST(TestCoreUtil, TestGetFirstOneBitPos)
{
    SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(uint8{ 0b1000'0000 }).value(), 7);
    SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(uint8{ 0b0100'0001 }).value(), 0);
    SOUL_TEST_ASSERT_FALSE(soul::util::get_first_one_bit_pos(uint8{ 0 }));

    SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(uint16{ 0b1000'0000'0000'0000 }).value(), 15);
    SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(uint16{ 0b0100'0100'0100'0001 }).value(), 0);
    SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(static_cast<uint16>(~uint16{0})).value(), 0);
    SOUL_TEST_ASSERT_FALSE(soul::util::get_first_one_bit_pos(uint16{ 0 }));

    SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(uint32{ 1 }).value(), 0);
    SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(uint32{ 0x80000000 }).value(), 31);
    SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(uint32{ 0x40000000 }).value(), 30);
    SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(~uint32{ 0 }).value(), 0);
    SOUL_TEST_ASSERT_FALSE(soul::util::get_first_one_bit_pos(uint32{ 0 }));

    SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(uint64{ 1 }).value(), 0);
    SOUL_TEST_ASSERT_FALSE(soul::util::get_first_one_bit_pos(uint64{ 0 }));
    SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(~uint64{ 0 }).value(), 0);
    SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(uint64{ 0x40000000 }).value(), 30);
    SOUL_TEST_ASSERT_EQ(soul::util::get_first_one_bit_pos(uint64{ 18446744069414584320llu }).value(), 32);

}

TEST(TestCoreUtil, TestGetLastOneBitPos)
{
    SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(uint8{ 0b1000'0000 }).value(), 7);
    SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(uint8{ 0b0100'0001 }).value(), 6);
    SOUL_TEST_ASSERT_FALSE(soul::util::get_last_one_bit_pos(uint8{ 0 }));

    SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(uint16{ 0b1000'0000'0000'0000 }).value(), 15);
    SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(uint16{ 0b0100'0100'0100'0001 }).value(), 14);
    SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(static_cast<uint16>(~uint16{ 0 })).value(), 15);
    SOUL_TEST_ASSERT_FALSE(soul::util::get_last_one_bit_pos(uint16{ 0 }));

    SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(uint32{ 1 }).value(), 0);
    SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(uint32{ 0x80000000 }).value(), 31);
    SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(uint32{ 0x400F0200 }).value(), 30);
    SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(~uint32{ 0 }).value(), 31);
    SOUL_TEST_ASSERT_FALSE(soul::util::get_last_one_bit_pos(uint32{ 0 }));

    SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(uint64{ 1 }).value(), 0);
    SOUL_TEST_ASSERT_FALSE(soul::util::get_last_one_bit_pos(uint64{ 0 }));
    SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(~uint64{ 0 }).value(), 63);
    SOUL_TEST_ASSERT_EQ(soul::util::get_last_one_bit_pos(uint64{ 0x4F000200 }).value(), 30);
}

TEST(TestCoreUtil, TestGetOneBitCount)
{
    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(uint8{ 0b1000'0000 }), 1);
    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(uint8{ 0b0100'0001 }), 2);
    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(uint8{ 0 }), 0);

    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(uint16{ 0b1000'0000'0000'0000 }), 1);
    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(uint16{ 0b0100'0100'0100'0001 }), 4);
    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(static_cast<uint16>(~uint16{ 0 })), 16);
    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(uint16{ 0 }), 0);

    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(uint32{ 1 }), 1);
    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(uint32{ 0x80000000 }), 1);
    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(uint32{ 0x400F0200 }), 6);
    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(~uint32{ 0 }), 32);
    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(uint32{ 0 }), 0);

    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(uint64{ 1 }), 1);
    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(uint64{ 0 }), 0);
    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(~uint64{ 0 }), 64);
    SOUL_TEST_ASSERT_EQ(soul::util::get_one_bit_count(uint64{ 0x4F000200 }), 6);
}

TEST(TestCoreUtil, TestForEachOneBitPos)
{
    auto test_for_each_one_bit_pos = []<std::integral T>(T val, const std::vector<uint32>& expected_value)
    {
        std::vector<uint32> bit_pos;
        soul::util::for_each_one_bit_pos(val, [&bit_pos](const soul_size position)
        {
            bit_pos.push_back(position);
        });

        SOUL_TEST_ASSERT_TRUE(std::ranges::equal(bit_pos, expected_value));
    };

    SOUL_TEST_RUN(test_for_each_one_bit_pos(uint8{ 0b1000'0000 }, std::vector<uint32>({7})));
    SOUL_TEST_RUN(test_for_each_one_bit_pos(uint8{ 0 }, std::vector<uint32>()));

    std::vector<uint32> expected_result_all_set(16);
    std::iota(std::begin(expected_result_all_set), std::end(expected_result_all_set), 0);
    SOUL_TEST_RUN(test_for_each_one_bit_pos(static_cast<uint16>(~uint16{0}), expected_result_all_set));

    SOUL_TEST_RUN(test_for_each_one_bit_pos(uint64{ 0x4F000200 }, std::vector<uint32>({9, 24, 25, 26, 27, 30})));

}