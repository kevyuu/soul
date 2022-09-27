#pragma once

#include "core/vector.h"

namespace soul
{

    static constexpr soul_size PREFERRED_SBO_VECTOR_SIZEOF = 64;
    template <typename T>
    static soul_size constexpr get_sbo_vector_default_inline_element_count()
    {
        static_assert(
            sizeof(T) <= 256,
            "You are trying to use a default number of inlined elements for "
            "`SBOVector<T>` but `sizeof(T)` is really big! Please use an "
            "explicit number of inlined elements with `SBOVector<T, N>` to make "
            "sure you really want that much inline storage.");
        constexpr auto preferred_inline_bytes =
            PREFERRED_SBO_VECTOR_SIZEOF - sizeof(Vector<T>);
        constexpr auto num_elements_that_fit = preferred_inline_bytes / sizeof(T);
        return num_elements_that_fit == 0 ? 1 : num_elements_that_fit;
    }

    template <typename T, soul_size N = get_sbo_vector_default_inline_element_count<T>(), memory::allocator_type AllocatorType = memory::Allocator>
    using SBOVector = Vector<T, AllocatorType, N>;

}
