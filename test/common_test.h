#pragma once

#include "core/array.h"
#include "core/hash.h"
#include "core/type.h"

#include "util.h"

using namespace soul;

template <typeset T>
auto test_copy_constructor(const T& result_src)
{
  const auto test = result_src;
  verify_equal(test, result_src);
}

template <typeset T>
auto test_clone(const T& result_src)
{
  const auto test = result_src.clone();
  verify_equal(test, result_src);
}

template <typeset T>
auto test_move_constructor(const T& result_src)
{
  auto result_src_clone = duplicate(result_src);
  const auto test = std::move(result_src_clone);
  verify_equal(test, result_src);
}

template <typeset T>
auto test_copy_assignment(const T& src, const T& sample_dst)
{
  auto dst = sample_dst;
  dst = src;
  verify_equal(dst, src);
}

template <typeset T>
auto test_clone_from(const T& src, const T& sample_dst)
{
  auto dst = sample_dst.clone();
  dst.clone_from(src);
  verify_equal(dst, src);
}

template <typeset T>
auto test_move_assignment(const T& sample_src, const T& sample_dst)
{
  auto src = duplicate(sample_src);
  auto dst = duplicate(sample_dst);
  dst = std::move(src);
  verify_equal(dst, sample_src);
}

template <typeset T>
auto test_swap(const T& sample_lhs, const T& sample_rhs)
{
  auto lhs = duplicate(sample_lhs);
  auto rhs = duplicate(sample_rhs);
  swap(lhs, rhs);
  verify_equal(rhs, sample_lhs);
  verify_equal(lhs, sample_rhs);
}

template <typename T, usize ArrSizeV>
auto test_hash_implementation(const soul::Array<T, ArrSizeV>& vals)
{
  for (usize idx1 = 0; idx1 < vals.size(); idx1++) {
    for (usize idx2 = 0; idx2 < vals.size(); idx2++) {
      if (idx1 == idx2) {
        SOUL_TEST_ASSERT_EQ(soul::hash(vals[idx1]), soul::hash(vals[idx2]));
      } else {
        SOUL_TEST_ASSERT_NE(soul::hash(vals[idx1]), soul::hash(vals[idx2]));
      }
    }
  }
}

template <typename T, usize ArrSizeV>
auto test_hash_span_implementation(const soul::Array<T, ArrSizeV>& vals)
{
  for (usize idx1 = 0; idx1 < vals.size(); idx1++) {
    for (usize idx2 = 0; idx2 < vals.size(); idx2++) {
      if (idx1 == idx2) {
        SOUL_TEST_ASSERT_EQ(soul::hash_span(vals[idx1]), soul::hash_span(vals[idx2]));
      } else {
        SOUL_TEST_ASSERT_NE(soul::hash_span(vals[idx1]), soul::hash_span(vals[idx2]));
      }
    }
  }
}
