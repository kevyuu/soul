// ReSharper disable CppInconsistentNaming
#pragma once
#include "core/type.h"
#include "core/compiler.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace soul::math
{

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
    T radians(T degrees)
    {
        return glm::radians(degrees);
    }

}