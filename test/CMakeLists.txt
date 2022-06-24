
add_executable(test_vector test_vector.cpp util.cpp)
target_link_libraries(test_vector
	PRIVATE GTest::gtest GTest::gtest_main soul
)

add_executable(test_intrusive_list test_intrusive_list.cpp util.cpp)
target_link_libraries(test_intrusive_list
	PRIVATE GTest::gtest GTest::gtest_main soul
)

add_executable(test_flag_set "test_flag_set.cpp" util.cpp)
target_link_libraries(test_flag_set
	PRIVATE GTest::gtest GTest::gtest_main soul)

add_test(gtest_vector test_vector)
add_test(gtest_intrusive_list test_intrusive_list)
add_test(gtest_flag_set test_flag_set)