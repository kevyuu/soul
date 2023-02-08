#include <random>
#include <set>

#include "core/bitset.h"
#include "util.h"

template<soul_size BitCount, soul::bit_block_type BlockType>
void verify_empty_bitset(const soul::Bitset<BitCount, BlockType> bitset)
{
    SOUL_TEST_ASSERT_FALSE(bitset.any());
    SOUL_TEST_ASSERT_FALSE(bitset.all());
    SOUL_TEST_ASSERT_TRUE(bitset.none());
    SOUL_TEST_ASSERT_EQ(bitset.count(), 0);
    SOUL_TEST_ASSERT_FALSE(bitset.find_first());
    SOUL_TEST_ASSERT_FALSE(bitset.find_last());
    SOUL_TEST_ASSERT_EQ(bitset.size(), BitCount);
    for (soul_size i = 0; i < bitset.size(); i++)
    {
        SOUL_TEST_ASSERT_FALSE(bitset.test(i)) << ", Index : " << i;
        SOUL_TEST_ASSERT_FALSE(bitset[i]) << ", Index : " << i;
    }
    for (soul_size i = 0; i < BitCount - 1; i++)
    {
        SOUL_TEST_ASSERT_FALSE(bitset.find_next(i));
        SOUL_TEST_ASSERT_FALSE(bitset.find_prev(BitCount - i));
    }
}

template<soul_size BitCount, soul::bit_block_type BlockType>
void verify_full_bitset(const soul::Bitset<BitCount, BlockType> bitset)
{
    SOUL_TEST_ASSERT_TRUE(bitset.any());
    SOUL_TEST_ASSERT_TRUE(bitset.all());
    SOUL_TEST_ASSERT_FALSE(bitset.none());
    SOUL_TEST_ASSERT_EQ(bitset.count(), BitCount);
    SOUL_TEST_ASSERT_EQ(bitset.size(), BitCount);
    for (soul_size i = 0; i < bitset.size(); i++)
    {
        SOUL_TEST_ASSERT_TRUE(bitset.test(i)) << ", Index : " << i;
        SOUL_TEST_ASSERT_TRUE(bitset[i]) << ", Index : " << i;
    }

    SOUL_TEST_ASSERT_EQ(*bitset.find_first(), 0);
    SOUL_TEST_ASSERT_EQ(*bitset.find_last(), BitCount - 1);
    for (soul_size i = 0; i < BitCount - 1; i++)
    {
        SOUL_TEST_ASSERT_EQ(*bitset.find_next(i), i + 1) << ", Index : " << i;
        SOUL_TEST_ASSERT_EQ(*bitset.find_prev(BitCount - i), BitCount - i - 1) << ", Index : " << i;
    }
    SOUL_TEST_ASSERT_FALSE(bitset.find_next(BitCount - 1));
    SOUL_TEST_ASSERT_FALSE(bitset.find_prev(0));
}

template<soul_size BitCount, soul::bit_block_type BlockType>
void verify_bitset(const soul::Bitset<BitCount, BlockType> bitset, const std::set<uint64>& positions) {
    for (soul_size position = 0; position < BitCount; position++)
    {
        const bool expected = positions.contains(position);
        SOUL_TEST_ASSERT_EQ(bitset.test(position), expected) << ", Position : " << position;
        SOUL_TEST_ASSERT_EQ(bitset[position], expected) << ", Position : " << position;
    }
    SOUL_TEST_ASSERT_EQ(bitset.size(), BitCount);
    SOUL_TEST_ASSERT_EQ(bitset.count(), positions.size());
    SOUL_TEST_ASSERT_EQ(bitset.all(), positions.size() == BitCount);
    SOUL_TEST_ASSERT_NE(bitset.any(), positions.empty());
    SOUL_TEST_ASSERT_EQ(bitset.none(), positions.empty());
}

template <soul_size BitCount, typename BlockType = void>
void test_bitset_constructor()
{
    if constexpr (std::is_void_v<BlockType>) {
        soul::Bitset<BitCount> bitset;
        verify_empty_bitset(bitset);
    }
    else
    {
        soul::Bitset<BitCount, BlockType> bitset;
        verify_empty_bitset(bitset);
    }
}

TEST(TestBitsetConstructor, TestBitSetDefaultConstructor)
{
    SOUL_TEST_RUN(test_bitset_constructor<1>());
    SOUL_TEST_RUN(test_bitset_constructor<8>());
    SOUL_TEST_RUN(test_bitset_constructor<17>());
    SOUL_TEST_RUN(test_bitset_constructor<32>());
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_bitset_constructor<10000, uint8>()));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_bitset_constructor<16, uint64>()));
}

template <soul_size BitCount, typename BlockTypeOptional = void>
void test_bitset_set(const std::set<uint64>& positions)
{
    using BlockType = std::conditional_t<std::is_void_v<BlockTypeOptional>,
        uint8,
        BlockTypeOptional>;
    using Bitset = std::conditional_t<std::is_void_v<BlockTypeOptional>,
        soul::Bitset<BitCount>,
        soul::Bitset<BitCount, BlockType>>;

    Bitset bitset;
    for (uint64 position : positions)
    {
        bitset.set(position);
        SOUL_TEST_ASSERT_TRUE(bitset.test(position));
        SOUL_TEST_ASSERT_TRUE(bitset[position]);
    }
}

TEST(TestBitsetSet, TestBitsetSet)
{
    SOUL_TEST_RUN(test_bitset_set<1>({0u}));
    SOUL_TEST_RUN(test_bitset_set<15>({ 1u, 7u, 14u }));
    SOUL_TEST_RUN(test_bitset_set<100>({ 0u, 99u }));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_bitset_set<7, uint64>({ 0u, 3u, 5u })));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_bitset_set<10000, uint8>({5u, 7u, 15u, 16u, 9999u})));
}

template <soul_size BitCount, soul::bit_block_type BlockType>
std::set<uint64> generate_position_set(const soul::Bitset<BitCount, BlockType>& bitset)
{
    std::set<uint64> result;
    for (uint32 position = 0; position < BitCount; position++)
    {
        if (bitset.test(position)) result.insert(position);
    }
    return result;
}

template <soul_size BitCount, soul::bit_block_type BlockType>
void generate_random_bitset(soul::Bitset<BitCount, BlockType>& bitset, const soul_size set_count)
{
    std::random_device random_device;
    std::mt19937 random_engine(random_device());
    std::vector<uint32> vec(BitCount);
    std::iota(vec.begin(), vec.end(), 0);
    std::ranges::shuffle(vec, random_engine);
    for (soul_size i = 0; i < set_count; i++)
    {
        bitset.set(vec[i]);
    }
}

template <soul_size BitCount, soul::bit_block_type BlockType = soul::default_block_type_t<BitCount>>
soul::Bitset<BitCount, BlockType> generate_bitset(const std::set<uint64>& positions)
{
    soul::Bitset<BitCount, BlockType> result;
    for (uint64 position : positions)
    {
        result.set(position);
    }
    return result;
}

class TestBitsetManipulation : public ::testing::Test
{
public:
    soul::Bitset<1000> empty_bitset;
    soul::Bitset<5> filled_bitset1;
    soul::Bitset<15> filled_bitset2;
    soul::Bitset<30> filled_bitset3;
    soul::Bitset<1000> filled_bitset4;

    TestBitsetManipulation()
    {
        generate_random_bitset(filled_bitset1, 2);
        generate_random_bitset(filled_bitset2, 8);
        generate_random_bitset(filled_bitset3, 30);
        generate_random_bitset(filled_bitset4, 200);
    }
};

TEST_F(TestBitsetManipulation, TestBitsetSetFalse)
{
    auto test_set_false = []<soul_size BitCount, soul::bit_block_type BlockType>
        (soul::Bitset<BitCount, BlockType> bitset, const std::set<uint64> removed_positions)
    {
        std::set<uint64> expected_set = generate_position_set(bitset);

        for (uint64 position : removed_positions) {
            bitset.set(position, false);
            expected_set.erase(position);
        }

        verify_bitset(bitset, expected_set);
    };

    SOUL_TEST_RUN(test_set_false(empty_bitset, { 1u, 3u, 5u, 999u }));
    SOUL_TEST_RUN(test_set_false(filled_bitset1, { *filled_bitset1.find_first() }));
    SOUL_TEST_RUN(test_set_false(filled_bitset1, { *filled_bitset1.find_last() }));
    SOUL_TEST_RUN(test_set_false(filled_bitset2, { *filled_bitset2.find_first(), 3u, 6u }));
    SOUL_TEST_RUN(test_set_false(filled_bitset3, { *filled_bitset3.find_first(), 3u, 6u }));

}

TEST_F(TestBitsetManipulation, TestBitsetSetAll)
{
    auto test_set_all = []<soul_size BitCount, soul::bit_block_type BlockType>
        (soul::Bitset<BitCount, BlockType> bitset)
    {
        bitset.set();
        verify_full_bitset(bitset);
    };
    
    SOUL_TEST_RUN(test_set_all(empty_bitset));
    SOUL_TEST_RUN(test_set_all(filled_bitset1));
    SOUL_TEST_RUN(test_set_all(filled_bitset2));
    SOUL_TEST_RUN(test_set_all(filled_bitset3));
    SOUL_TEST_RUN(test_set_all(filled_bitset3));
}

TEST_F(TestBitsetManipulation, TestBitsetReset)
{
    auto test_reset = []<soul_size BitCount, soul::bit_block_type BlockType>
        (soul::Bitset<BitCount, BlockType> bitset)
    {
        bitset.reset();
        verify_empty_bitset(bitset);
    };

    SOUL_TEST_RUN(test_reset(empty_bitset));
    SOUL_TEST_RUN(test_reset(filled_bitset1));
    SOUL_TEST_RUN(test_reset(filled_bitset2));
    SOUL_TEST_RUN(test_reset(filled_bitset3));
    SOUL_TEST_RUN(test_reset(filled_bitset3));
}

TEST_F(TestBitsetManipulation, TestBitsetFlip)
{
    auto test_flip = []<soul_size BitCount, soul::bit_block_type BlockType>
        (soul::Bitset<BitCount, BlockType> bitset)
    {
        std::set<uint64> expected_result;
        for (soul_size i = 0; i < BitCount; i++)
        {
            if (!bitset.test(i))
                expected_result.insert(i);
        }
        bitset.flip();
        verify_bitset(bitset, expected_result);
    };

    SOUL_TEST_RUN(test_flip(empty_bitset));
    SOUL_TEST_RUN(test_flip(filled_bitset1));
    SOUL_TEST_RUN(test_flip(filled_bitset2));
    SOUL_TEST_RUN(test_flip(filled_bitset3));
    SOUL_TEST_RUN(test_flip(filled_bitset3));
}

TEST(TestBitsetOperator, TestBitsetOperatorNegate)
{
    auto test_operator_negate = []<soul_size BitCount, soul::bit_block_type BlockType>
        (soul::Bitset<BitCount, BlockType> bitset)
    {
        std::set<uint64> expected_result;
        for (soul_size i = 0; i < BitCount; i++)
        {
            if (!bitset.test(i))
                expected_result.insert(i);
        }
        verify_bitset(~bitset, expected_result);
    };

    SOUL_TEST_RUN(test_operator_negate(generate_bitset<100>({ 0, 99 })));
    SOUL_TEST_RUN(test_operator_negate(generate_bitset<100>({})));
    SOUL_TEST_RUN(test_operator_negate(generate_bitset<10000>({ 0, 4, 10, 63, 9999 })));
    SOUL_TEST_RUN(test_operator_negate(generate_bitset<5>({ 2 })));
}

TEST(TestBitsetOperator, TestBitsetOperatorAnd)
{
    auto test_operator_and = []<soul_size BitCount, soul::bit_block_type BlockType>
        (soul::Bitset<BitCount, BlockType> bitset1, soul::Bitset<BitCount, BlockType> bitset2, const std::set<uint64> expected_result)
    {
        soul::Bitset<BitCount, BlockType> bitset_result = bitset1 & bitset2;
        verify_bitset(bitset_result, expected_result);
        bitset1 &= bitset2;
        verify_bitset(bitset1, expected_result);
        SOUL_TEST_ASSERT_TRUE(bitset_result == bitset1);
    };

    SOUL_TEST_RUN(test_operator_and(generate_bitset<100>({ 0, 99 }), generate_bitset<100>({ 2, 3 }), {}));
    SOUL_TEST_RUN(test_operator_and(generate_bitset<100>({ 2, 99 }), generate_bitset<100>({ 2, 3 }), {2}));
    SOUL_TEST_RUN(test_operator_and(generate_bitset<10000>({ 0, 4, 10, 63, 9999 }), generate_bitset<10000>({ 2, 3, 7, 10, 63, 9999 }), {10, 63, 9999}));
    SOUL_TEST_RUN(test_operator_and(generate_bitset<5>({}), generate_bitset<5>({ 2, 3 }), {}));
    SOUL_TEST_RUN(test_operator_and(generate_bitset<5>({}), generate_bitset<5>({}), {}));

}

TEST(TestBitsetOperator, TestBitsetOperatorOr)
{
    auto test_operator_or = []<soul_size BitCount, soul::bit_block_type BlockType>
        (soul::Bitset<BitCount, BlockType> bitset1, soul::Bitset<BitCount, BlockType> bitset2, const std::set<uint64> expected_result)
    {
        soul::Bitset<BitCount, BlockType> bitset_result = bitset1 | bitset2;
        verify_bitset(bitset_result, expected_result);
        bitset1 |= bitset2;
        verify_bitset(bitset1, expected_result);
        SOUL_TEST_ASSERT_TRUE(bitset_result == bitset1);
    };

    SOUL_TEST_RUN(test_operator_or(generate_bitset<100>({ 0, 99 }), generate_bitset<100>({ 2, 3 }), {0, 2, 3, 99}));
    SOUL_TEST_RUN(test_operator_or(generate_bitset<100>({ 2, 99 }), generate_bitset<100>({ 2, 3 }), { 2, 3, 99 }));
    SOUL_TEST_RUN(test_operator_or(generate_bitset<10000>({ 0, 4, 10, 63, 9999 }), generate_bitset<10000>({ 2, 3, 7, 10, 63, 9999 }), { 0, 2, 3, 4, 7, 10, 63, 9999 }));
    SOUL_TEST_RUN(test_operator_or(generate_bitset<5>({}), generate_bitset<5>({ 2, 3 }), {2, 3}));
    SOUL_TEST_RUN(test_operator_or(generate_bitset<5>({}), generate_bitset<5>({}), {}));
}

TEST(TestBitsetOperator, TestBitsetOperatorXor)
{
    auto test_operator_xor = []<soul_size BitCount, soul::bit_block_type BlockType>
        (soul::Bitset<BitCount, BlockType> bitset1, soul::Bitset<BitCount, BlockType> bitset2)
    {
        std::set<uint64> expected_result;
        for (soul_size i = 0; i < BitCount; i++)
        {
            if (bitset1.test(i) != bitset2.test(i))
                expected_result.insert(i);
        }
        soul::Bitset<BitCount, BlockType> bitset_result = bitset1 ^ bitset2;
        verify_bitset(bitset_result, expected_result);
        bitset1 ^= bitset2;
        verify_bitset(bitset1, expected_result);
        SOUL_TEST_ASSERT_TRUE(bitset_result == bitset1);
    };

    SOUL_TEST_RUN(test_operator_xor(generate_bitset<100>({ 0, 99 }), generate_bitset<100>({ 2, 3 })));
    SOUL_TEST_RUN(test_operator_xor(generate_bitset<100>({ 2, 99 }), generate_bitset<100>({ 2, 3 })));
    SOUL_TEST_RUN(test_operator_xor(generate_bitset<10000>({ 0, 4, 10, 63, 9999 }), generate_bitset<10000>({ 2, 3, 7, 10, 63, 9999 })));
    SOUL_TEST_RUN(test_operator_xor(generate_bitset<5>({}), generate_bitset<5>({ 2, 3 })));
    SOUL_TEST_RUN(test_operator_xor(generate_bitset<5>({}), generate_bitset<5>({})));
    SOUL_TEST_RUN(test_operator_xor(generate_bitset<5>({}), ~generate_bitset<5>({})));
}

TEST(TestBitsetForEach, TestBitsetForEach)
{
    auto test_for_each = []<soul_size BitCount, soul::bit_block_type BlockType>(soul::Bitset<BitCount, BlockType> bitset) {

        std::vector<soul_size> expected_positions;
        for (soul_size i = 0; i < BitCount; i++)
        {
            if (bitset.test(i)) expected_positions.push_back(i);
        }

        std::vector<soul_size> positions;
        bitset.for_each([&positions](soul_size position)
        {
            positions.push_back(position);
        });

        SOUL_TEST_ASSERT_TRUE(std::ranges::equal(positions, expected_positions));
    };

    SOUL_TEST_RUN(test_for_each(generate_bitset<100>({ 0, 99 })));
    SOUL_TEST_RUN(test_for_each(generate_bitset<100>({})));
    SOUL_TEST_RUN(test_for_each(generate_bitset<10000>({ 0, 4, 10, 63, 9999 })));
    SOUL_TEST_RUN(test_for_each(generate_bitset<5>({ 2 })));
}

TEST(TestBitSetFindIf, TestBitSetFindIf)
{
    auto test_find_if = []<soul_size BitCount, soul::bit_block_type BlockType>(soul::Bitset<BitCount, BlockType> bitset, std::vector<soul_size> test_positions) {

        std::vector<soul_size> expected_positions;
        for (soul_size i = 0; i < BitCount; i++)
        {
            if (bitset.test(i)) expected_positions.push_back(i);
        }

        std::vector<soul_size> positions;
        bitset.for_each([&positions](soul_size position)
            {
                positions.push_back(position);
            });

        SOUL_TEST_ASSERT_TRUE(std::ranges::equal(positions, expected_positions));

        for (soul_size position : test_positions)
        {
            const auto find_result = bitset.find_if([position](soul_size bit) { return bit == position; });
            if (position < BitCount && bitset.test(position))
                SOUL_TEST_ASSERT_EQ(find_result.value(), position);
            else
                SOUL_TEST_ASSERT_FALSE(find_result.has_value());
        }
    };

    SOUL_TEST_RUN(test_find_if(generate_bitset<100>({ 0, 99 }), { 0, 3, 5, 99 }));
    SOUL_TEST_RUN(test_find_if(generate_bitset<100>({}), { 1, 3, 5, 102 }));
    SOUL_TEST_RUN(test_find_if(generate_bitset<10000>({ 0, 4, 10, 63, 9999 }), { 4, 7, 63, 9998, 9999 }));
    SOUL_TEST_RUN(test_find_if(generate_bitset<5>({ 2 }), { 1, 2, 3 }));
}

TEST(TestBitsetToUint, TestBitsetToUint)
{
    SOUL_TEST_ASSERT_EQ(generate_bitset<10>({}).to_uint32(), 0);
    SOUL_TEST_ASSERT_EQ(generate_bitset<10>({ 0, 9 }).to_uint32(), 513);
    SOUL_TEST_ASSERT_EQ(generate_bitset<32>({ 0, 9 }).to_uint32(), 513);

    SOUL_TEST_ASSERT_EQ(generate_bitset<10>({}).to_uint64(), 0u);
    SOUL_TEST_ASSERT_EQ(generate_bitset<10>({ 0, 9 }).to_uint64(), 513u);
    SOUL_TEST_ASSERT_EQ(generate_bitset<33>({ 0, 9 }).to_uint64(), 513u);
    SOUL_TEST_ASSERT_EQ(generate_bitset<64>({ 0, 9, 34 }).to_uint64(), 17179869697u);

    // test fail compilation. Uncomment tests below, expected to generate compilation error since
    // the bitcount exceeded the width of the uint
    //SOUL_TEST_ASSERT_EQ(generate_bitset<40>({ 0, 4 }).to_uint32(), 17);
    //SOUL_TEST_ASSERT_EQ(generate_bitset<100>({ 0, 3 }).to_uint64(), 9);
}