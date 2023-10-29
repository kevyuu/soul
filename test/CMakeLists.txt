add_executable(test_meta test_meta.cpp util.cpp)
target_link_libraries(test_meta PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_core_util test_util.cpp util.cpp)
target_link_libraries(test_core_util PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_array test_array.cpp util.cpp)
target_link_libraries(test_array PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_vector test_vector.cpp util.cpp)
target_link_libraries(test_vector PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_option test_option.cpp util.cpp)
target_link_libraries(test_option PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_maybe_null test_maybe_null.cpp util.cpp)
target_link_libraries(test_maybe_null PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_result test_result.cpp util.cpp)
target_link_libraries(test_result PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_not_null test_not_null.cpp util.cpp)
target_link_libraries(test_not_null PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_intrusive_list test_intrusive_list.cpp util.cpp)
target_link_libraries(test_intrusive_list PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_flag_set "test_flag_set.cpp" util.cpp)
target_link_libraries(test_flag_set PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_bit_vector test_bit_vector.cpp util.cpp)
target_link_libraries(test_bit_vector PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_bitset "test_bitset.cpp" util.cpp)
target_link_libraries(test_bitset PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_sbo_vector "test_sbo_vector.cpp" util.cpp)
target_link_libraries(test_sbo_vector PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_cstring test_cstring.cpp util.cpp)
target_link_libraries(test_cstring PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_variant test_variant.cpp util.cpp)
target_link_libraries(test_variant PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_tuple test_tuple.cpp util.cpp)
target_link_libraries(test_tuple PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_hash test_hash.cpp util.cpp)
target_link_libraries(test_hash PRIVATE GTest::gtest GTest::gtest_main soul)

add_executable(test_robin_table test_robin_table.cpp util.cpp)
target_link_libraries(test_robin_table PRIVATE GTest::gtest GTest::gtest_main soul)

add_test(gtest_meta test_meta)
add_test(gtest_core_util test_core_util)
add_test(gtest_array test_array)
add_test(gtest_vector test_vector)
add_test(gtest_option test_option)
add_test(gtest_maybe_null test_maybe_null)
add_test(gtest_result test_result)
add_test(gtest_not_null test_not_null)
add_test(gtest_intrusive_list test_intrusive_list)
add_test(gtest_flag_set test_flag_set)
add_test(gtest_bit_vector test_bit_vector)
add_test(gtest_bitset test_bitset)
add_test(gtest_sbo_vector test_sbo_vector)
add_test(gtest_cstring test_cstring)
add_test(gtest_variant test_variant)
add_test(gtest_tuple test_tuple)
add_test(gtest_hash test_hash)
add_test(gtest_robin_table test_robin_table)
