#include <algorithm>
#include <random>

#define USE_CUSTOM_DEFAULT_ALLOCATOR
#include "core/config.h"

#include "core/sbo_vector.h"
#include "memory/allocator.h"
#include <gtest/gtest.h>
#include "util.h"
#include <list>
#include <numeric>


namespace soul
{
    memory::Allocator* get_default_allocator()
    {
        static TestAllocator test_allocator("Test default allocator");
        return &test_allocator;
    }
}

using ListTestObject = std::list<TestObject>;
using VectorInt = soul::SBOVector<int>;
using VectorObj = soul::SBOVector<TestObject, 4>;
using VectorListObj = soul::SBOVector<ListTestObject>;

constexpr soul_size CONSTRUCTOR_VECTOR_SIZE = 10;
constexpr int CONSTRUCTOR_VECTOR_DEFAULT_VALUE = 7;

template <typename T, soul_size N>
bool all_equal(soul::SBOVector<T, N>& vec, const T& val)
{
    return std::ranges::all_of(vec, [val](const T& x) { return x == val; });
}

template <typename T, soul_size N>
void verify_sbo_vector(const soul::SBOVector<T, N>& vec1, const soul::SBOVector<T,N>& vec2)
{
    SOUL_TEST_ASSERT_EQ(vec1.size(), vec2.size());
    SOUL_TEST_ASSERT_EQ(vec1.empty(), vec2.empty());
    if (!vec1.empty())
    {
        SOUL_TEST_ASSERT_EQ(vec1.front(), vec2.front());
        SOUL_TEST_ASSERT_EQ(vec1.back(), vec2.back());
    }
    SOUL_TEST_ASSERT_TRUE(std::equal(vec1.begin(), vec1.end(), vec2.begin()));
}

template <typename T, soul_size N>
soul::SBOVector<T, N> generate_random_sbo_vector(const soul_size size)
{
    const auto sequence = generate_random_sequence<T>(size);
    soul::SBOVector<T, N> vector(sequence.begin(), sequence.end());
    return vector;
}

template <typename T, soul_size N>
void verify_sbo_vector(const soul::SBOVector<T, N>& vec, const Sequence<T>& sequence)
{
    SOUL_TEST_ASSERT_EQ(vec.size(), sequence.size());
    SOUL_TEST_ASSERT_EQ(vec.empty(), sequence.empty());
    if (!sequence.empty())
    {
        SOUL_TEST_ASSERT_EQ(vec.front(), *sequence.begin());
        SOUL_TEST_ASSERT_EQ(vec.back(), *(sequence.end() - 1));
    }
    SOUL_TEST_ASSERT_TRUE(std::equal(sequence.begin(), sequence.end(), vec.begin()));

}

template<typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_constructor()
{
    soul::SBOVector<T,N> vector;
    SOUL_TEST_ASSERT_EQ(vector.size(), 0);
    SOUL_TEST_ASSERT_TRUE(vector.empty());
}

TEST(TestSBOVectorConstructor, TestDefaultConstructor)
{
    SOUL_TEST_RUN(test_constructor<int>());
    SOUL_TEST_RUN(test_constructor<TestObject>());
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_constructor<TestObject, 0>()));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_constructor<TestObject, 4>()));
    SOUL_TEST_RUN(test_constructor<ListTestObject>());
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_constructor<ListTestObject, 0>()));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_constructor<ListTestObject, 4>()));
}

TEST(TestSBOVectorConstructor, TestCustomAllocatorConstructor)
{
    TestObject::reset();
    TestAllocator::reset_all();
    TestAllocator test_allocator;

    static constexpr auto INLINE_ELEMENT_COUNT = 8;
    soul::SBOVector<int, INLINE_ELEMENT_COUNT,TestAllocator> vec_int(&test_allocator);
    ASSERT_TRUE(vec_int.empty());

    soul::SBOVector<TestObject, INLINE_ELEMENT_COUNT, TestAllocator> vec_to(&test_allocator);
    ASSERT_TRUE(vec_to.empty());

    soul::SBOVector<ListTestObject, INLINE_ELEMENT_COUNT, TestAllocator> vec_list_to(&test_allocator);
    ASSERT_TRUE(vec_list_to.empty());

    vec_int.resize(INLINE_ELEMENT_COUNT + 1);
    vec_to.resize(INLINE_ELEMENT_COUNT + 1);
    vec_list_to.resize(INLINE_ELEMENT_COUNT + 1);
    ASSERT_EQ(TestAllocator::allocCountAll, 3);

}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_constructor_with_size(const soul_size size)
{
    soul::SBOVector<T,N> vector(size);
    SOUL_TEST_ASSERT_EQ(vector.size(), size);
    SOUL_TEST_ASSERT_TRUE(all_equal(vector, T()));

}

TEST(TestSBOVectorConstructor, TestConstructorWithSize)
{
    SOUL_TEST_RUN(test_constructor_with_size<int>(CONSTRUCTOR_VECTOR_SIZE));
    SOUL_TEST_RUN(test_constructor_with_size<TestObject>(CONSTRUCTOR_VECTOR_SIZE));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_constructor_with_size<TestObject, 4>(CONSTRUCTOR_VECTOR_SIZE)));
    SOUL_TEST_RUN(test_constructor_with_size<ListTestObject>(CONSTRUCTOR_VECTOR_SIZE));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_constructor_with_size<ListTestObject, 4>(CONSTRUCTOR_VECTOR_SIZE)));

    SOUL_TEST_RUN(test_constructor_with_size<int>(0));
    SOUL_TEST_RUN(test_constructor_with_size<TestObject>(0));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_constructor_with_size<TestObject, 4>(0)));
    SOUL_TEST_RUN(test_constructor_with_size<ListTestObject>(0));
    SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_constructor_with_size<ListTestObject, 4>(0)));

}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_constructor_with_size_and_value(const soul_size size, const T& val)
{
    soul::SBOVector<T,N> vector(size, val);
    SOUL_TEST_ASSERT_EQ(vector.size(), size);
    SOUL_TEST_ASSERT_TRUE(all_equal(vector, val));
    SOUL_TEST_ASSERT_EQ(vector.empty(), size == 0);
    if (size != 0)
    {
        SOUL_TEST_ASSERT_EQ(vector.front(), val);
        SOUL_TEST_ASSERT_EQ(vector.back(), val);
    }
}

TEST(TestSBOVectorConstructor, TestConstrucorWithSizeAndValue)
{
    SOUL_TEST_RUN(test_constructor_with_size_and_value(CONSTRUCTOR_VECTOR_SIZE, CONSTRUCTOR_VECTOR_DEFAULT_VALUE));
    SOUL_TEST_RUN(test_constructor_with_size_and_value(CONSTRUCTOR_VECTOR_SIZE, TestObject(CONSTRUCTOR_VECTOR_DEFAULT_VALUE)));
    SOUL_TEST_RUN(test_constructor_with_size_and_value(CONSTRUCTOR_VECTOR_SIZE, ListTestObject(CONSTRUCTOR_VECTOR_SIZE)));
}

class TestSBOVectorConstructorWithSourceData : public ::testing::Test
{
public:
    VectorInt vector_int_src{ CONSTRUCTOR_VECTOR_SIZE, CONSTRUCTOR_VECTOR_DEFAULT_VALUE };
    soul::SBOVector <TestObject> vector_to_src{ CONSTRUCTOR_VECTOR_SIZE, TestObject(CONSTRUCTOR_VECTOR_DEFAULT_VALUE) };
    soul::SBOVector <ListTestObject> vector_list_to_src{ CONSTRUCTOR_VECTOR_SIZE, ListTestObject(CONSTRUCTOR_VECTOR_DEFAULT_VALUE) };
};

TEST_F(TestSBOVectorConstructorWithSourceData, TestCopyConstructor)
{
    auto test_copy_constructor = []<typename T, soul_size N>(const soul::SBOVector<T,N>& vector_src) {
        soul::SBOVector<T,N> vector_dst(vector_src);  // NOLINT(performance-unnecessary-copy-initialization)
        verify_sbo_vector(vector_dst, vector_src);
    };

    SOUL_TEST_RUN(test_copy_constructor(vector_int_src));
    SOUL_TEST_RUN(test_copy_constructor(vector_to_src));
    SOUL_TEST_RUN(test_copy_constructor(vector_list_to_src));

}

TEST_F(TestSBOVectorConstructorWithSourceData, TestCopyConstructorWithCustomAllocator)
{
    auto test_constructor_with_custom_allocator = []<typename T, soul_size N>(const soul::SBOVector<T,N>&vector_src) {
        TestAllocator::reset_all();
        TestAllocator test_allocator;

        SOUL_TEST_ASSERT_EQ(test_allocator.allocCount, 0);
        soul::SBOVector<T,N> vector_dst(vector_src, test_allocator);
        verify_sbo_vector(vector_dst, vector_src);
        SOUL_TEST_ASSERT_EQ(test_allocator.allocCount, 1);
    };

    SOUL_TEST_RUN(test_constructor_with_custom_allocator(vector_int_src));
    SOUL_TEST_RUN(test_constructor_with_custom_allocator(vector_to_src));
    SOUL_TEST_RUN(test_constructor_with_custom_allocator(vector_list_to_src));
}

TEST_F(TestSBOVectorConstructorWithSourceData, TestMoveConstructor)
{
    auto test_move_constructor = []<typename T, soul_size N>(const soul::SBOVector<T,N>&vector_src) {
        soul::SBOVector<T,N> vector_src_copy(vector_src);
        soul::SBOVector<T,N> vector_dst(std::move(vector_src_copy));
        verify_sbo_vector(vector_dst, vector_src);
    };

    SOUL_TEST_RUN(test_move_constructor(vector_int_src));
    SOUL_TEST_RUN(test_move_constructor(vector_to_src));
    SOUL_TEST_RUN(test_move_constructor(vector_list_to_src));

}

TEST_F(TestSBOVectorConstructorWithSourceData, TestIteratorConstructor)
{
    auto test_iterator_constructor = []<typename T, soul_size N>(const soul::SBOVector<T,N>&vector_src) {
        soul::SBOVector<T,N> vector_dst(vector_src.begin(), vector_src.end());
        verify_sbo_vector(vector_dst, vector_src);
    };

    SOUL_TEST_RUN(test_iterator_constructor(vector_int_src));
    SOUL_TEST_RUN(test_iterator_constructor(vector_to_src));
    SOUL_TEST_RUN(test_iterator_constructor(vector_list_to_src));

}

TEST_F(TestSBOVectorConstructorWithSourceData, TestIteratorConstructorWithCustomAllocator)
{
    auto test_iterator_constructor_with_custom_allocator = []<typename T, soul_size N>(const soul::SBOVector<T,N> vector_src) {
        TestAllocator test_allocator;

        soul::SBOVector<T,N> vector_dst(vector_src.begin(), vector_src.end(), test_allocator);
        verify_sbo_vector(vector_dst, vector_src);
        SOUL_TEST_ASSERT_EQ(test_allocator.allocCount, 1);
    };

    SOUL_TEST_RUN(test_iterator_constructor_with_custom_allocator(vector_int_src));
    SOUL_TEST_RUN(test_iterator_constructor_with_custom_allocator(vector_to_src));
    SOUL_TEST_RUN(test_iterator_constructor_with_custom_allocator(vector_list_to_src));
}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_vector_getter(const soul_size size)
{

    SOUL_TEST_ASSERT_NE(size, 0);
    const auto sequence = generate_random_sequence<T>(size);

    const auto middle = size / 2;

    soul::SBOVector<T,N> vector(sequence.begin(), sequence.end());
    SOUL_TEST_ASSERT_EQ(vector.front(), *sequence.begin());
    SOUL_TEST_ASSERT_EQ(vector.back(), *(sequence.end() - 1));
    SOUL_TEST_ASSERT_EQ(vector[middle], *(sequence.begin() + middle));
}

TEST(TestSBOVectorGetter, TestSBOVectorGetter)
{
    SOUL_TEST_RUN(test_vector_getter<int>(7));
    SOUL_TEST_RUN(test_vector_getter<TestObject>(9));
    SOUL_TEST_RUN(test_vector_getter<ListTestObject>(7));
}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_set_allocator(const soul_size vec_size)
{
    const auto sequence = generate_random_sequence<T>(vec_size);
    soul::SBOVector<T, N> vector(sequence.begin(), sequence.end());
    TestAllocator test_allocator;
    vector.set_allocator(test_allocator);
    SOUL_TEST_ASSERT_EQ(vector.get_allocator(), &test_allocator);
    verify_sbo_vector(vector, sequence);
}

TEST(TestSBOVectorSetAllocator, TestSBOVectorSetAllocator)
{

    auto type_set_test = []<typename T>() {

        SOUL_TEST_RUN(test_set_allocator<T>(0));
        SOUL_TEST_RUN(test_set_allocator<T>(20));
        SOUL_TEST_RUN(test_set_allocator<T>(1));

        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_set_allocator<T, 8>(0)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_set_allocator<T, 8>(1)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_set_allocator<T, 8>(8)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_set_allocator<T, 8>(10)));
    };

    SOUL_TEST_RUN(type_set_test.operator() < int > ());
    SOUL_TEST_RUN(type_set_test.operator() < TestObject > ());
    SOUL_TEST_RUN(type_set_test.operator() < ListTestObject > ());
}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_copy_assignment_operator(const soul_size src_size, const soul_size dst_size)
{
    TestAllocator test_allocator("Test Allocator For Copy Assignment Operator");
    const auto src_sequence = generate_random_sequence<T>(src_size);
    const auto dst_sequence = generate_random_sequence<T>(dst_size);

    soul::SBOVector<T, N> src_vector(src_sequence.begin(), src_sequence.end());
    soul::SBOVector<T, N> dst_vector(dst_sequence.begin(), dst_sequence.end(), test_allocator);

    dst_vector = src_vector;

    verify_sbo_vector(dst_vector, src_sequence);
    verify_sbo_vector(src_vector, src_sequence);
    SOUL_TEST_ASSERT_EQ(dst_vector.get_allocator(), &test_allocator);
};

TEST(TestSBOVectorCopyAssignmentOperator, TestSBOVectorCopyAssignmentOperator)
{
    auto type_set_test = []<typename T>() {
        
        auto src_size_set_test = [](const soul_size src_size)
        {
            constexpr auto default_inline_count = soul::get_sbo_vector_default_inline_element_count<T>();
            SOUL_TEST_RUN(test_copy_assignment_operator<T>(src_size, std::max<soul_size>(default_inline_count / 2, 1)));
            SOUL_TEST_RUN(test_copy_assignment_operator<T>(src_size, default_inline_count * 2));
            SOUL_TEST_RUN(test_copy_assignment_operator<T>(src_size, default_inline_count));

            constexpr auto custom_inline_count = 8;
            SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_copy_assignment_operator<T, custom_inline_count>(src_size, custom_inline_count / 2)));
            SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_copy_assignment_operator<T, custom_inline_count>(src_size, custom_inline_count * 2)));
            SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_copy_assignment_operator<T, custom_inline_count>(src_size, custom_inline_count)));

        };

        SOUL_TEST_RUN(src_size_set_test(0));
        SOUL_TEST_RUN(src_size_set_test(6));
        SOUL_TEST_RUN(src_size_set_test(8));
        SOUL_TEST_RUN(src_size_set_test(100));

    };
    
    SOUL_TEST_RUN(type_set_test.operator()<int>());
    SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
    SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_move_assignment_operator(const soul_size src_size, const soul_size dst_size)
{
    TestAllocator test_allocator("Test Allocator For Move Assignment Operator");
    const auto src_sequence = generate_random_sequence<T>(src_size);
    const auto dst_sequence = generate_random_sequence<T>(dst_size);

    soul::SBOVector<T, N> src_vector(src_sequence.begin(), src_sequence.end());
    soul::SBOVector<T, N> dst_vector(dst_sequence.begin(), dst_sequence.end(), test_allocator);

    dst_vector = std::move(src_vector);

    verify_sbo_vector(dst_vector, src_sequence);
    SOUL_TEST_ASSERT_EQ(dst_vector.get_allocator(), &test_allocator);
};

TEST(TestSBOVectorMoveAssingmentOperator, TestSBOVectorMoveAssignmentOperator)
{
    auto type_set_test = []<typename T>() {

        auto src_size_set_test = [](const soul_size src_size)
        {
            constexpr auto default_inline_count = soul::get_sbo_vector_default_inline_element_count<T>();
            SOUL_TEST_RUN(test_move_assignment_operator<T>(src_size, std::max<soul_size>(default_inline_count / 2, 1)));
            SOUL_TEST_RUN(test_move_assignment_operator<T>(src_size, default_inline_count * 2));
            SOUL_TEST_RUN(test_move_assignment_operator<T>(src_size, default_inline_count));

            constexpr auto custom_inline_count = 8;
            SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_move_assignment_operator<T, custom_inline_count>(src_size, custom_inline_count / 2)));
            SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_move_assignment_operator<T, custom_inline_count>(src_size, custom_inline_count * 2)));
            SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_move_assignment_operator<T, custom_inline_count>(src_size, custom_inline_count)));

        };

        SOUL_TEST_RUN(src_size_set_test(0));
        SOUL_TEST_RUN(src_size_set_test(6));
        SOUL_TEST_RUN(src_size_set_test(8));
        SOUL_TEST_RUN(src_size_set_test(100));
    };

    SOUL_TEST_RUN(type_set_test.operator() < int > ());
    SOUL_TEST_RUN(type_set_test.operator() < TestObject > ());
    SOUL_TEST_RUN(type_set_test.operator() < ListTestObject > ());
}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_assign_with_size_and_value(const soul_size vec_size, const soul_size assign_size, const T assign_val)
{
    auto vector = generate_random_sbo_vector<T, N>(vec_size);
    vector.assign(assign_size, assign_val);
    verify_sbo_vector(vector, generate_sequence(assign_size, assign_val));
}

TEST(TestSBOVectorAssignWithSizeAndValue, TestSBOVectorAssignWithSizeAndValue)
{
    static constexpr auto ASSIGN_VECTOR_DEFAULT_VALUE = 8;

    auto type_set_test = []<typename T>()
    {
        constexpr auto default_inline_element_count = soul::get_sbo_vector_default_inline_element_count<T>();
        const T default_val(ASSIGN_VECTOR_DEFAULT_VALUE);
        
        SOUL_TEST_RUN(test_assign_with_size_and_value<T>(1,default_inline_element_count, default_val));
        SOUL_TEST_RUN(test_assign_with_size_and_value<T>(1, default_inline_element_count * 2, default_val));

        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_size_and_value<T, 8>(0, 8, default_val)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_size_and_value<T, 8>(0, 16, default_val)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_size_and_value<T, 8>(1, 8, default_val)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_size_and_value<T, 8>(8, 6, default_val)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_size_and_value<T, 8>(8, 16, default_val)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_size_and_value<T, 8>(16, 8, default_val)));
    };

    SOUL_TEST_RUN(type_set_test.operator()<int>());
    SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
    SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_assign_with_iterator(const soul_size vec_size, const soul_size assign_size)
{
    auto vector = generate_random_sbo_vector<T, N>(vec_size);
    const auto sequence = generate_random_sequence<T>(assign_size);
    vector.assign(sequence.begin(), sequence.end());
    verify_sbo_vector(vector, sequence);
}

TEST(TestSBOVectorAssignWithIterator, TestSBOVectorAssignWithIterator)
{

    auto type_set_test = []<typename T>() {
        constexpr auto default_inline_element_count = soul::get_sbo_vector_default_inline_element_count<T>();
        SOUL_TEST_RUN(test_assign_with_iterator<T>(0, default_inline_element_count));
        SOUL_TEST_RUN(test_assign_with_iterator<T>(0, default_inline_element_count * 2));

        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_iterator<T, 8>(4, 2)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_iterator<T, 8>(4, 8)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_iterator<T, 8>(4, 16)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_iterator<T, 8>(10, 2)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_iterator<T, 8>(10, 9)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_assign_with_iterator<T, 8>(10, 16)));
    };

    SOUL_TEST_RUN(type_set_test.operator()<int>());
    SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
    SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_swap(const soul_size size1, const soul_size size2)
{
    const auto sequence1 = generate_random_sequence<T>(size1);
    const auto sequence2 = generate_random_sequence<T>(size2);

    soul::SBOVector<T, N> vector1(sequence1.begin(), sequence1.end());
    soul::SBOVector<T, N> vector2(sequence2.begin(), sequence2.end());
    vector1.swap(vector2);
    verify_sbo_vector(vector1, sequence2);
    verify_sbo_vector(vector2, sequence1);

    swap(vector1, vector2);
    verify_sbo_vector(vector1, sequence1);
    verify_sbo_vector(vector2, sequence2);
}

TEST(TestSBOVectorSwap, TestSBOVectorSwap)
{
    auto type_set_test = []<typename T>() {
        constexpr auto default_inline_element_count = soul::get_sbo_vector_default_inline_element_count<T>();

        SOUL_TEST_RUN(test_swap<T>(0, default_inline_element_count));
        SOUL_TEST_RUN(test_swap<T>(0, default_inline_element_count * 2));

        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_swap<T, 8>(2, 3)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_swap<T, 8>(2, 16)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_swap<T, 8>(16, 32)));
    };

    SOUL_TEST_RUN(type_set_test.operator()<int>());
    SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
    SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_resize(const soul_size vec_size, const soul_size resize_size)
{
    const auto original_sequence = generate_random_sequence<T>(vec_size);
    soul::SBOVector<T, N> vector(original_sequence.begin(), original_sequence.end());

    const auto resize_sequence = [vec_size, resize_size, &original_sequence]()
    {
        if (resize_size > vec_size)
        {
            return generate_sequence(original_sequence, generate_sequence(resize_size - vec_size, T()));
        }
        return generate_sequence<T>(original_sequence.begin(), original_sequence.begin() + resize_size);
        
    }();

    if constexpr (std::same_as<T, TestObject> || std::same_as<T, ListTestObject>)
    {
        TestObject::reset();
    }
    vector.resize(resize_size);
    verify_sbo_vector(vector, resize_sequence);
    if (vec_size > resize_size)
    {
        if constexpr (std::same_as<T, TestObject>)
        {
            SOUL_TEST_ASSERT_EQ(TestObject::sTODtorCount - TestObject::sTOCtorCount, vec_size - resize_size);
        }
        if constexpr (std::same_as<T, ListTestObject>)
        {
            const auto destructed_objects_count = std::accumulate(original_sequence.begin() + resize_size, original_sequence.end(), 0,
                [](const soul_size prev, const ListTestObject& curr) { return prev + curr.size(); });
            SOUL_TEST_ASSERT_EQ(TestObject::sTODtorCount - TestObject::sTOCtorCount, destructed_objects_count);
        }
    }
}

TEST(TestSBOVectorResize, TestSBOVectorResize)
{
    auto type_set_test = []<typename T>() {
        constexpr auto default_inline_element_count = soul::get_sbo_vector_default_inline_element_count<T>();

        SOUL_TEST_RUN(test_resize<T>(0, default_inline_element_count));
        SOUL_TEST_RUN(test_resize<T>(0, default_inline_element_count * 2));
        SOUL_TEST_RUN(test_resize<T>(default_inline_element_count, 0));
        SOUL_TEST_RUN(test_resize<T>(default_inline_element_count, 1));
        SOUL_TEST_RUN(test_resize<T>(default_inline_element_count * 2, 0));
        SOUL_TEST_RUN(test_resize<T>(default_inline_element_count * 2, default_inline_element_count));

        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(0, 8)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(0, 16)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(8, 0)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(8, 4)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(8, 16)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(6, 8)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(10, 2)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(10, 9)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_resize<T, 8>(10, 16)));
    };
    
    SOUL_TEST_RUN(type_set_test.operator()<int>());
    SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
    SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_reserve(const soul_size vec_size, const soul_size new_capacity)
{
    const auto sequence = generate_random_sequence<T>(vec_size);
    soul::SBOVector<T, N> vector(sequence.begin(), sequence.end());
    const auto old_capacity = vector.capacity();
    vector.reserve(new_capacity);
    verify_sbo_vector(vector, sequence);
    if (old_capacity >= new_capacity)
    {
        SOUL_TEST_ASSERT_EQ(vector.capacity(), old_capacity);
    }
    else
    {
        SOUL_TEST_ASSERT_EQ(vector.capacity(), new_capacity);
    }
}

TEST(TestSBOVectorReserve, TestSBOVectorReserve)
{
    auto type_set_test = []<typename T>() {
        constexpr auto default_inline_element_count = soul::get_sbo_vector_default_inline_element_count<T>();
        SOUL_TEST_RUN(test_reserve<T>(0, default_inline_element_count - 1));
        SOUL_TEST_RUN(test_reserve<T>(0, default_inline_element_count));
        SOUL_TEST_RUN(test_reserve<T>(0, default_inline_element_count * 2));

        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_reserve<T, 8>(4, 8)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_reserve<T, 8>(4, 16)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_reserve<T, 8>(4, 2)));

        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_reserve<T, 8>(16, 8)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_reserve<T, 8>(16, 32)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_reserve<T, 8>(16, 2)));

    };

    SOUL_TEST_RUN(type_set_test.operator()<int>());
    SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
    SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_push_back(const soul_size vec_size, const T& val)
{
    using Vector = soul::SBOVector<T, N>;
    const auto sequence = generate_random_sequence<T>(vec_size);
    Vector vector(sequence.begin(), sequence.end());

    const auto push_back_sequence = generate_sequence(sequence, generate_sequence(1, val));

    Vector test_copy1(vector);
    Vector test_copy2(vector);
    Vector test_copy3(vector);

    test_copy1.push_back(val);
    verify_sbo_vector(test_copy1, push_back_sequence);

    T val_copy = val;
    test_copy2.push_back(std::move(val_copy));
    verify_sbo_vector(test_copy2, push_back_sequence);

    test_copy3.push_back();
    verify_sbo_vector(test_copy3, generate_sequence(sequence, generate_sequence(1, T())));
}

TEST(TestSBOVectorPushback, TestSBOVectorPushBack)
{
    auto type_set_test = []<typename T>()
    {
        SOUL_TEST_RUN(test_push_back<T>(0, T(5)));

        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_push_back<T, 8>(0, T(5))));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_push_back<T, 8>(7, T(5))));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_push_back<T, 8>(8, T(5))));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_push_back<T, 8>(12, T(5))));
    };

    SOUL_TEST_RUN(type_set_test.operator()<int>());
    SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
    SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_pop_back(const soul_size vec_size)
{
    const auto sequence = generate_random_sequence<T>(vec_size);
    soul::SBOVector<T, N> vector(sequence.begin(), sequence.end());
    if constexpr (std::same_as<T, TestObject>)
    {
        TestObject::reset();
    }
    vector.pop_back();
    verify_sbo_vector(vector, generate_sequence<T>(sequence.begin(), sequence.end() - 1));
    if constexpr (std::same_as<T, TestObject>)
    {
        SOUL_TEST_ASSERT_EQ(TestObject::sTODtorCount - TestObject::sTOCtorCount, 1);
    }
}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_pop_back_with_size(const soul_size vec_size, const soul_size pop_back_size)
{
    const auto sequence = generate_random_sequence<T>(vec_size);
    soul::SBOVector<T, N> vector(sequence.begin(), sequence.end());
    if constexpr (std::same_as<T, TestObject>)
    {
        TestObject::reset();
    }
    vector.pop_back(pop_back_size);
    verify_sbo_vector(vector, generate_sequence<T>(sequence.begin(), sequence.end() - pop_back_size));
    if constexpr (std::same_as<T, TestObject>)
    {
        SOUL_TEST_ASSERT_EQ(TestObject::sTODtorCount - TestObject::sTOCtorCount, pop_back_size);
    }
}

TEST(TestSBOVectorPopBack, TestSBOVectorPopBack)
{
    auto type_set_test = []<typename T>()
    {
        constexpr auto default_inline_element_count = soul::get_sbo_vector_default_inline_element_count<T>();
        SOUL_TEST_RUN(test_pop_back<T>(1));
        SOUL_TEST_RUN(test_pop_back<T>(default_inline_element_count));
        SOUL_TEST_RUN(test_pop_back<T>(default_inline_element_count + 1));

        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_pop_back<T, 8>(8)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_pop_back<T, 8>(9)));

        SOUL_TEST_RUN(test_pop_back_with_size<T>(2, 2));
        SOUL_TEST_RUN(test_pop_back_with_size<T>(default_inline_element_count + 2, default_inline_element_count + 2));
        SOUL_TEST_RUN(test_pop_back_with_size<T>(default_inline_element_count + 2, default_inline_element_count + 1));

        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_pop_back_with_size<T, 8>(4, 4)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_pop_back_with_size<T, 8>(4, 2)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_pop_back_with_size<T, 8>(16, 8)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_pop_back_with_size<T, 8>(16, 4)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_pop_back_with_size<T, 8>(16, 16)));
    };

    SOUL_TEST_RUN(type_set_test.operator() < int > ());
    SOUL_TEST_RUN(type_set_test.operator() < TestObject > ());
    SOUL_TEST_RUN(type_set_test.operator() < ListTestObject > ());

}

TEST(TestSBOVectorEmplaceBack, TestSBOVectorEmplaceBack)
{
    auto test_emplace_back = []<soul_size N>(const soul_size vec_size)
    {
        const auto vector_sequence = generate_random_sequence<TestObject>(vec_size);
        soul::SBOVector<TestObject, N> vector(vector_sequence.begin(), vector_sequence.end());

        soul::SBOVector<TestObject, N> test_copy1(vector);
        test_copy1.emplace_back(3);
        SOUL_TEST_ASSERT_EQ(test_copy1.size(), vector.size() + 1);
        SOUL_TEST_ASSERT_EQ(test_copy1.back(), TestObject(3));
        SOUL_TEST_ASSERT_TRUE(std::equal(vector.cbegin(), vector.cend(), test_copy1.cbegin()));

        soul::SBOVector<TestObject, N> test_copy2(vector);
        test_copy2.emplace_back(4, 5, 6);
        SOUL_TEST_ASSERT_EQ(test_copy2.size(), vector.size() + 1);
        SOUL_TEST_ASSERT_EQ(test_copy2.back(), TestObject(4 + 5 + 6));
        SOUL_TEST_ASSERT_TRUE(std::equal(vector.cbegin(), vector.cend(), test_copy2.cbegin()));
    };

    SOUL_TEST_RUN(test_emplace_back.operator()<1>(0));
    SOUL_TEST_RUN(test_emplace_back.operator()<8>(7));
    SOUL_TEST_RUN(test_emplace_back.operator()<8>(8));
}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_append(const soul_size vec_size, const soul_size append_size)
{
    const auto vector_sequence = generate_random_sequence<T>(vec_size);
    soul::SBOVector<T, N> vector(vector_sequence.begin(), vector_sequence.end());

    const auto append_sequence = generate_random_sequence<T>(append_size);

    using Vector = soul::SBOVector<T, N>;

    Vector test_copy1(vector);
    test_copy1.append(append_sequence.begin(), append_sequence.end());
    SOUL_TEST_ASSERT_EQ(test_copy1.size(), vector.size() + append_size);
    SOUL_TEST_ASSERT_TRUE(std::equal(vector.begin(), vector.end(), test_copy1.begin()));
    SOUL_TEST_ASSERT_TRUE(std::equal(test_copy1.begin() + vector.size(), test_copy1.end(), append_sequence.begin(), append_sequence.end()));

    Vector append_src_vec(append_sequence.begin(), append_sequence.end());
    Vector test_copy2(vector);
    test_copy2.append(append_src_vec);
    SOUL_TEST_ASSERT_EQ(test_copy2.size(), test_copy1.size());
    SOUL_TEST_ASSERT_TRUE(std::ranges::equal(test_copy1, test_copy2));
}

TEST(TestSBOVectorAppend, TestSBOVectorAppend)
{

    auto type_set_test = []<typename T>()
    {
        SOUL_TEST_RUN(test_append<T>(0, 1));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_append<T, 8>(3, 2)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_append<T, 8>(3, 4)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_append<T, 8>(3, 5)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_append<T, 8>(3, 16)));
    };

    SOUL_TEST_RUN(type_set_test.operator() < int > ());
    SOUL_TEST_RUN(type_set_test.operator() < TestObject > ());
    SOUL_TEST_RUN(type_set_test.operator() < ListTestObject > ());

}
template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_clear(const soul_size size)
{
    const auto sequence = generate_random_sequence<T>(size);
    soul::SBOVector<T, N> vector(sequence.begin(), sequence.end());

    const soul_size old_capacity = vector.capacity();
    const soul_size old_size = vector.size();
    if constexpr (std::same_as<T, TestObject>)
    {
        TestObject::reset();
    }
    vector.clear();
    SOUL_TEST_ASSERT_EQ(vector.size(), 0);
    SOUL_TEST_ASSERT_EQ(vector.capacity(), old_capacity);
    if constexpr (std::same_as<T, TestObject>)
    {
        SOUL_TEST_ASSERT_EQ(TestObject::sTODtorCount - TestObject::sTOCtorCount, old_size);
    }
    
}

TEST(TestSBOVectorClear, TestSBOVectorClear)
{
    auto type_set_test = []<typename T>() {
        SOUL_TEST_RUN(test_clear<T>(0));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_clear<T, 8>(4)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_clear<T, 8>(8)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_clear<T, 8>(16)));
    };

    SOUL_TEST_RUN(type_set_test.operator() < int > ());
    SOUL_TEST_RUN(type_set_test.operator() < TestObject > ());
    SOUL_TEST_RUN(type_set_test.operator() < ListTestObject > ());
}

template <typename T, soul_size N = soul::get_sbo_vector_default_inline_element_count<T>()>
void test_cleanup(const soul_size size)
{
    const auto sequence = generate_random_sequence<T>(size);
    soul::SBOVector<T, N> vector(sequence.begin(), sequence.end());

    const soul_size old_size = vector.size();
    using VecType = soul::SBOVector<T, N>;
    if constexpr (std::same_as<T, TestObject>)
    {
        TestObject::reset();
    }
    vector.cleanup();
    SOUL_TEST_ASSERT_EQ(vector.size(), 0);
    SOUL_TEST_ASSERT_EQ(vector.capacity(), VecType::INLINE_ELEMENT_COUNT);
    if constexpr (std::same_as<T, TestObject>)
    {
        SOUL_TEST_ASSERT_EQ(TestObject::sTODtorCount - TestObject::sTOCtorCount, old_size);
    }
}

TEST(TestSBOVectorCleanup, TestSBOVectorCleanup)
{
    auto type_set_test = []<typename T>() {
        SOUL_TEST_RUN(test_cleanup<T>(0));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_cleanup<T, 8>(4)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_cleanup<T, 8>(8)));
        SOUL_TEST_RUN(SOUL_SINGLE_ARG(test_cleanup<T, 8>(16)));
    };

    SOUL_TEST_RUN(type_set_test.operator()<int>());
    SOUL_TEST_RUN(type_set_test.operator()<TestObject>());
    SOUL_TEST_RUN(type_set_test.operator()<ListTestObject>());
}

