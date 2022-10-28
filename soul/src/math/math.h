// ReSharper disable CppInconsistentNaming
#pragma once
#include "core/type.h"
#include "core/compiler.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace soul::math
{

    namespace fconst
    {
        static constexpr auto PI = 3.14f;
    }

    template <std::integral T>
    float fdiv(T a, T b)
    {
        return static_cast<float>(a) / static_cast<float>(b);
    }

    template <soul_size D1, soul_size D2, soul_size D3, typename T>
    SOUL_ALWAYS_INLINE mat4f mul(const matrix<D1, D2, T>& a, const matrix<D2, D3, T>& b)
    {
        return mat4f(a.mat * b.mat);
    }

    template <typename T>
    SOUL_ALWAYS_INLINE matrix4x4<T> translate(const matrix4x4<T>& m, const vec3<T>& v)
    {
        return matrix4x4<T>(glm::translate(m.mat, v));
    }

    template <typename T>
    SOUL_ALWAYS_INLINE matrix4x4<T> rotate(const matrix4x4<T>& m, T angle, const vec3<T>& axis)
    {
        return matrix4x4<T>(glm::rotate(m.mat, angle, axis));
    }

    template <typename T>
    SOUL_ALWAYS_INLINE matrix4x4<T> scale(const matrix4x4<T>& m, const vec3<T>& scale)
    {
        return matrix4x4<T>(glm::scale(m.mat, scale));
    }

    template <typename T>
    SOUL_ALWAYS_INLINE matrix4x4<T> look_at(const vec3<T>& eye, const vec3<T>& center, const vec3<T>& up)
    {
        return mat4f(glm::lookAt(eye, center, up));
    }

    template <typename T>
    SOUL_ALWAYS_INLINE matrix4x4<T> perspective(T fovy, T aspect, T z_near, T z_far)
    {
        return mat4f(glm::perspective(fovy, aspect, z_near, z_far));
    }

    template <soul_size Row, soul_size Column, typename T>
    SOUL_ALWAYS_INLINE matrix<Row, Column, T> inverse(const matrix<Row, Column, T>& mat)
    {
        return mat4f(glm::inverse(mat.mat));
    }

    template <soul_size Row, soul_size Column, typename T>
    SOUL_ALWAYS_INLINE matrix<Row, Column, T> transpose(const matrix<Row, Column, T>& mat)
    {
        return mat4f(glm::transpose(mat.mat));
    }

    template <typename T>
    T radians(T degrees)
    {
        return glm::radians(degrees);
    }

    template <soul_size Dim, typename T>
    SOUL_ALWAYS_INLINE soul::vec<Dim, T> normalize(const soul::vec<Dim, T>& x)
    {
        return glm::normalize(x);
    }

    template <typename T>
    SOUL_ALWAYS_INLINE soul::vec<3, T> cross(const soul::vec<3, T>& x, const soul::vec<3, T>& y)
    {
        return glm::cross(x, y);
    }

    template <soul_size Dim, typename T>
    SOUL_ALWAYS_INLINE T dot(const soul::vec<Dim, T>& x, const soul::vec<Dim, T>& y)
    {
        return glm::dot(x, y);
    }

    template <typename T>
    SOUL_ALWAYS_INLINE T min(T x, T y)
    {
        return glm::min(x, y);
    }

    template <typename T>
    SOUL_ALWAYS_INLINE T max(T x, T y)
    {
        return glm::max(x, y);
    }

    SOUL_ALWAYS_INLINE AABB combine(const AABB aabb, const vec3f point)
    {
        return { min(aabb.min, point), max(aabb.max, point) };
    }

    SOUL_ALWAYS_INLINE AABB combine(const AABB x, const AABB y)
    {
        return { min(x.min, y.min), max(x.max, y.max) };
    }
}