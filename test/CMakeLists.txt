
add_executable(test_vector test_vector.cpp util.cpp)
target_link_libraries(test_vector
	PRIVATE GTest::gtest GTest::gtest_main soul
)

add_test(gtest_vector test_vector)