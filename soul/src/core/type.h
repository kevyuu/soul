//
// Created by Kevin Yudi Utama on 2/8/18.
//

// ReSharper disable CppInconsistentNaming
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <tl/expected.hpp>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "core/dev_util.h"
#include "core/type_traits.h"

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using bool32 = int32;

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using intptr = intptr_t;
using uintptr = uintptr_t;

using memory_index = size_t;

using byte = uint8;
using soul_size = uint64_t;

namespace soul {

	constexpr soul_size ONE_KILOBYTE = 1024;
	constexpr soul_size ONE_MEGABYTE = 1024 * ONE_KILOBYTE;
	constexpr soul_size ONE_GIGABYTE = 1024 * ONE_MEGABYTE;

	template <typename Val, typename Err>
	using expected = tl::expected<Val, Err>;

	template <class E>
	tl::unexpected<typename std::decay<E>::type> make_unexpected(E&& e) {
		return unexpected<typename std::decay<E>::type>(std::forward<E>(e));
	}

	template <typename T>
	concept bit_block_type = std::unsigned_integral<T>;

	template <typename T, soul_size N>
	struct RawBuffer {
		alignas(T) std::byte buffer[sizeof(T) * N];
		T* data()
		{
			return reinterpret_cast<T*>(buffer);
		}

		const T* data() const
		{
			return reinterpret_cast<const T*>(buffer);
		}
	};

	template <typename T>
	struct RawBuffer<T, 0>
	{
	    T* data()
	    {
			return nullptr;
	    }

		const T* data() const
	    {
			return nullptr;
	    }
	};

	template <soul_size Dim, typename T>
	using vec = glm::vec<Dim, T, glm::defaultp>;

	template <typename T>
	using vec2 = vec<2, T>;

	template <typename T>
	using vec3 = vec<3, T>;

	template <typename T>
	using vec4 = vec<4, T>;

	using vec2f = vec2<float>;
	using vec3f = vec3<float>;
	using vec4f = vec4<float>;

	using vec2d = vec2<double>;
	using vec3d = vec3<double>;
	using vec4d = vec4<double>;

	using vec2i16 = vec2<int16>;
	using vec3i16 = vec3<int16>;
	using vec4i16 = vec4<int16>;

	using vec2ui32 = vec2<uint32>;
	using vec3ui32 = vec3<uint32>;
	using vec4ui32 = vec4<uint32>;

	using vec2i32 = vec2<int32>;
	using vec3i32 = vec3<int32>;
	using vec4i32 = vec4<int32>;

	template <soul_size Row, soul_size Column, typename T>
	struct matrix
	{
		using this_type = matrix < Column, Row, T>;
		using store_type = glm::mat<Column, Row, T, glm::defaultp>;
		store_type mat;

		constexpr matrix() = default;
		constexpr explicit matrix(T val) : mat(val) {}
		constexpr explicit matrix(store_type mat) : mat(mat) {}

		SOUL_ALWAYS_INLINE [[nodiscard]] float m(const uint8 row, const uint8 column) const
		{
			return mat[column][row];
		}

		SOUL_ALWAYS_INLINE float& m(const uint8 row, const uint8 column)
		{
			return mat[column][row];
		}

		static constexpr this_type identity()
		{
			return this_type(store_type(1));
		}


	};

	template<typename T>
	using matrix4x4 = matrix<4, 4, T>;

	using mat3f = matrix<3, 3, float>;
	using mat4f = matrix<4, 4, float>;

	template <arithmetic T>
	struct Quaternion {
		union {
			struct { T x, y, z, w; };
			vec4<T> xyzw;
			vec3<T> xyz;
			vec2<T> xy;
			struct {
				vec3<T> vector;
				T real;
			};
			T mem[4];
		};

		constexpr Quaternion() noexcept : x(0), y(0), z(0), w(1) {}
		constexpr Quaternion(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {}
		constexpr Quaternion(vec3<T> xyz, T w) noexcept : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
		constexpr explicit Quaternion(const T* val) noexcept : x(val[0]), y(val[1]), z(val[2]), w(val[3]) {}
	};
	using Quaternionf = Quaternion<float>;

	struct Transformf {
		vec3f position;
		vec3f scale;
		Quaternionf rotation;
	};


	struct AABB {
		vec3f min = vec3f(std::numeric_limits<float>::max());
		vec3f max = vec3f(std::numeric_limits<float>::lowest());

		AABB() = default;
		AABB(const vec3f& min, const vec3f& max) noexcept : min{min}, max{max} {}
		[[nodiscard]] bool is_empty() const { return (min.x >= max.x || min.y >= max.y || min.z >= max.z); }
		[[nodiscard]] bool is_inside(const vec3f& point) const
		{
			return (point.x >= min.x && point.x <= max.x) && (point.y >= min.y && point.y <= max.y) && (point.z >= min.z && point.z <= max.z);
		}

		struct Corners
		{
			static constexpr soul_size COUNT = 8;
			vec3f vertices[COUNT];
		};

		[[nodiscard]] Corners get_corners() const
		{
			return {
				vec3f(min.x, min.y, min.z),
				vec3f(min.x, min.y, max.z),
				vec3f(min.x, max.y, min.z),
				vec3f(min.x, max.y, max.z),
				vec3f(max.x, min.y, min.z),
				vec3f(max.x, min.y, max.z),
				vec3f(max.x, max.y, min.z),
				vec3f(max.x, max.y, max.z)
			};
		}

		[[nodiscard]] vec3f center() const
		{
			return (min + max) / 2.0f;
		}

	};

	template<
		typename PointerDst,
		typename PointerSrc
	>
	requires std::is_pointer_v<PointerDst> && std::is_pointer_v<PointerSrc>
	constexpr PointerDst cast(PointerSrc srcPtr)
	{
		using Dst = std::remove_pointer_t<PointerDst>;
		if constexpr (!std::is_same_v<PointerDst, void*>) {
			SOUL_ASSERT(0, reinterpret_cast<uintptr>(srcPtr) % alignof(Dst) == 0, "Source pointer is not aligned in PointerDst alignment!");
		}
		return static_cast<PointerDst>(srcPtr);
	}

	template<
		std::integral IntegralDst,
		std::integral IntegralSrc
	>
	constexpr IntegralDst cast(IntegralSrc src) {
		SOUL_ASSERT(0, static_cast<uint64>(src) <= std::numeric_limits<IntegralDst>::max(), "Source value is larger than the destintation type maximum!");
		SOUL_ASSERT(0, static_cast<int64>(src) >= std::numeric_limits<IntegralDst>::min(), "Source value is smaller than the destination type minimum!");
	    return static_cast<IntegralDst>(src);
	}

	template<
		typename PointerDst,
		typename PointerSrc
	>
	requires std::is_pointer_v<PointerDst>&& std::is_pointer_v<PointerSrc>
	constexpr PointerDst downcast(PointerSrc srcPtr)
	{
		return static_cast<PointerDst>(srcPtr);
	}

	template <scoped_enum E>
	constexpr auto to_underlying(E e) noexcept
	{
		return static_cast<std::underlying_type_t<E>>(e);
	}

	template <
		typename ResourceType,
		typename IDType,
		IDType NullValue = std::numeric_limits<IDType>::max()
	>
	struct ID {

		using store_type = IDType;
		IDType id;

		constexpr ID() : id(NullValue) {}
		constexpr explicit ID(IDType id) : id(id) {}

		template<std::integral Integral>
		constexpr explicit ID(Integral id) : id(soul::cast<IDType>(id)) {}

		template<typename Pointer> requires std::is_pointer_v<Pointer>
		constexpr explicit ID(Pointer id): id(id) {}

		bool operator==(const ID& other) const { return other.id == id; }
		bool operator!=(const ID& other) const { return other.id != id; }
		bool operator<(const ID& other) const { return id < other.id; }
		bool operator<=(const ID& other) const { return id <= other.id; }
		[[nodiscard]] bool is_null() const { return id == NullValue; }
		[[nodiscard]] bool is_valid() const { return id != NullValue; }

		static constexpr ID null() {
			return ID(NullValue);
		}
	};

	template<flag Flag>
	class FlagIter {

		using store_type = std::underlying_type_t<Flag>;

	public:
		class Iterator {
		public:
			constexpr explicit Iterator(store_type index): index_(index) {}
			constexpr Iterator operator++() { ++index_; return *this; }
			constexpr bool operator!=(const Iterator& other) const { return index_ != other.index_; }
			constexpr Flag operator*() const { return Flag(index_); }
		private:
			store_type index_;
		};
		[[nodiscard]] Iterator begin() const { return Iterator(0);}
		[[nodiscard]] Iterator end() const { return Iterator(to_underlying(Flag::COUNT)); }

		constexpr FlagIter() = default;

		static FlagIter Iterates() {
			return FlagIter();
		}

		static uint64 Count() {
			return to_underlying(Flag::COUNT);
		}

	};

	template<typename InputIt, typename OutputIt>
	void Copy(InputIt first, InputIt last, OutputIt output)
	{
		std::copy(first, last, output);
	}

	template <
		typename ElemType
	>
	requires std::is_trivially_move_constructible_v<ElemType>
	void Move(ElemType* begin, ElemType* end, ElemType* dst) {
		memcpy((void*)dst, (void*)begin, uint64(end - begin) * sizeof(ElemType));  // NOLINT(bugprone-sizeof-expression)
	}

	template<
		typename ElemType
	>
	requires (!std::is_trivially_move_constructible_v<ElemType>)
	void Move(ElemType* begin, ElemType* end, ElemType* dst) {
		for (ElemType* iter = begin; iter != end; ++iter, ++dst) {
			new (dst) ElemType(std::move(*iter));
		}
	}

	template<
		typename ElemType
	>
	requires(is_trivially_destructible_v<ElemType>)
	void Destruct(ElemType* begin, ElemType* end) {}

	template<
		typename ElemType
	>
	requires(!is_trivially_destructible_v<ElemType>)
	void Destruct(ElemType* begin, ElemType* end) {
		for (ElemType* iter = begin; iter != end; ++iter) {
			iter->~ElemType();
		}
	}

	template <std::integral Integral, flag Enum>
	constexpr auto operator<<(Integral integral, Enum e)
	{
		using ReturnType = min_uint_t<1u << to_underlying(Enum::COUNT)>;
		return static_cast<ReturnType>(1u << to_underlying(e));
	}
}

