//
// Created by Kevin Yudi Utama on 2/8/18.
//

// ReSharper disable CppInconsistentNaming
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "core/compiler.h"
#include "core/dev_util.h"
#include "core/type_traits.h"

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef intptr_t intptr;
typedef uintptr_t uintptr;

typedef size_t memory_index;

typedef uint8 byte;
typedef uint64_t soul_size;

#define SOUL_ARRAY_LEN(arr) (sizeof(arr) / sizeof(*(arr)))
#define SOUL_BIT_COUNT(type) (sizeof(type) * 8)
#define SOUL_UTYPE_MAX(type) ((1u << SOUL_BIT_COUNT(type)) - 1)

namespace Soul {

	template <
		typename T,
		typename = require<is_numeric_v<T>>
	>
	struct Vec2 {
		T x = 0;
		T y = 0;

		Vec2() = default;
		constexpr Vec2(T x, T y) noexcept : x(x), y(y) {}
	};

	template <
		typename T,
		typename = require<is_numeric_v<T>>
	>
	struct Vec3 {
		union {
			struct { T x, y, z; };
			Vec2<T> xy;
			T mem[3];
		};

		constexpr Vec3() noexcept : x(0), y(0), z(0) {}
		constexpr Vec3(T x, T y, T z) noexcept : x(x), y(y), z(z) {}
		constexpr explicit Vec3(const T* val) noexcept : x(val[0]), y(val[1]), z(val[2]) {}
	};

	template <
		typename T,
		typename = require<is_numeric_v<T>>
	>
	struct Vec4 {
		union {
			struct { T x, y, z, w; };
			Vec3<T> xyz;
			Vec2<T> xy;
			T mem[4];
		};

		constexpr Vec4() noexcept : x(0), y(0), z(0), w(0) {}
		constexpr Vec4(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {}
		constexpr Vec4(Vec3<T> xyz, T w) noexcept : xyz(xyz), w(w) {}
		constexpr explicit Vec4(const T* val) noexcept : x(val[0]), y(val[1]), z(val[2]), w(val[3]) {}
	};

	using Vec2f = Vec2<float>;
	using Vec3f = Vec3<float>;
	using Vec4f = Vec4<float>;

	using Vec2ui16 = Vec2<uint16>;
	using Vec3ui16 = Vec3<uint16>;

	using Vec2ui32 = Vec2<uint32>;
	using Vec3ui32 = Vec3<uint32>;
	using Vec4ui32 = Vec4<uint32>;

	using Vec2i32 = Vec2<int32>;
	using Vec3i32 = Vec3<int32>;
	using Vec4i32 = Vec4<int32>;

	template <
		typename T,
		typename = require<is_numeric_v<T>>
	>
	struct Quaternion {
		union {
			struct { T x, y, z, w; };
			Vec4<T> xyzw;
			Vec3<T> xyz;
			Vec2<T> xy;
			struct {
				Vec3<T> vector;
				T real;
			};
			T mem[4];
		};

		constexpr Quaternion() noexcept : x(0), y(0), z(0), w(1) {}
		constexpr Quaternion(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {}
		constexpr Quaternion(Vec3<T> xyz, T w) noexcept : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
		constexpr explicit Quaternion(const T* val) noexcept : x(val[0]), y(val[1]), z(val[2]), w(val[3]) {}
	};
	using Quaternionf = Quaternion<float>;

	struct Transformf {
		Vec3f position;
		Vec3f scale;
		Quaternionf rotation;
	};

	struct Mat4f {
		union {
			float elem[4][4] = {};
			float mem[16];
			Vec4f rows[4];
		};
		constexpr Mat4f() noexcept : mem() {}
	};

	struct Mat3f {
		union {
			float elem[3][3] = {};
			float mem[9];
			Vec3f rows[3];
		};
		constexpr Mat3f() : mem() {}
		constexpr explicit Mat3f(const Mat4f& srcMat) noexcept : mem {
			srcMat.elem[0][0], srcMat.elem[0][1], srcMat.elem[0][2],
			srcMat.elem[1][0], srcMat.elem[1][1], srcMat.elem[1][2],
			srcMat.elem[2][0], srcMat.elem[2][1], srcMat.elem[2][2] } {}
	};

	struct GLSLMat3f {
		
		static constexpr int FLOAT_COUNT = 12;
		union {
			float mem[FLOAT_COUNT] = {};
			Vec4f col[3];
		};
		constexpr GLSLMat3f() noexcept : mem() {}

		constexpr explicit GLSLMat3f(const Mat3f& mat) noexcept : mem{ 
			mat.elem[0][0], mat.elem[1][0], mat.elem[2][0], 0.0f, 
			mat.elem[0][1], mat.elem[1][1], mat.elem[2][1], 0.0f,
			mat.elem[0][2], mat.elem[1][2], mat.elem[2][2], 0.0f}
		{}
	};

	struct GLSLMat4f {
		
		static constexpr int FLOAT_COUNT = 16;
		union {
			float mem[FLOAT_COUNT] = {};
			Vec4f col[4];
		};
        constexpr GLSLMat4f() : mem() {}

		constexpr explicit GLSLMat4f(const Mat4f& mat) noexcept : mem{
			mat.elem[0][0], mat.elem[1][0], mat.elem[2][0], mat.elem[3][0],
			mat.elem[0][1], mat.elem[1][1], mat.elem[2][1], mat.elem[3][1],
			mat.elem[0][2], mat.elem[1][2], mat.elem[2][2], mat.elem[3][2],
			mat.elem[0][3], mat.elem[1][3], mat.elem[2][3], mat.elem[3][3] }
		{}

		GLSLMat4f& operator=(const Mat4f& mat) noexcept
        {
			*this = GLSLMat4f(mat);
			return *this;
        }
	};

	struct AABB {
		Vec3f min = Vec3f(FLT_MAX, FLT_MAX, FLT_MAX);
		Vec3f max = Vec3f(FLT_MIN, FLT_MIN, FLT_MIN);

		AABB() = default;
		AABB(const Vec3f& min, const Vec3f& max) noexcept : min{min}, max{max} {}

	};

	template <
		typename ResourceType,
		typename IDType,
		IDType NULLVAL = std::numeric_limits<IDType>::max()
	>
	struct ID {
		using Type = IDType;
		IDType id;

		constexpr ID() : id(NULLVAL) {}
		constexpr explicit ID(IDType id) : id(id) {}
		bool operator==(const ID& other) const { return other.id == id; }
		bool operator!=(const ID& other) const { return other.id != id; }
		bool operator<(const ID& other) const { return id < other.id; }
		bool operator<=(const ID& other) const { return id <= other.id; }
		SOUL_NODISCARD bool isNull() const { return id == NULLVAL; }
	};

	template<typename T>
	class EnumIter {

	public:
		class Iterator {
		public:
			explicit Iterator(uint64 index): _index(index) {}
			Iterator operator++() { ++_index; return *this; }
			bool operator!=(const Iterator& other) const { return _index != other._index; }
			T operator*() const { return T(_index); }
		private:
			uint64 _index;
		};
		SOUL_NODISCARD Iterator begin() const { return Iterator(0);}
		SOUL_NODISCARD Iterator end() const { return Iterator(uint64(T::COUNT)); }

		static EnumIter Iterates() {
			return EnumIter();
		}

		static uint64 Count() {
			return uint64(T::COUNT);
		}

	private:
		EnumIter() = default;

	};

	template<typename InputIt, typename OutputIt>
	void Copy(InputIt first, InputIt last, OutputIt output)
	{
		std::copy(first, last, output);
	}

	template <
		typename ElemType,
		SOUL_REQUIRE(is_trivially_move_constructible_v<ElemType>)
	>
	void Move(ElemType* begin, ElemType* end, ElemType* dst) {
		memcpy((void*)dst, (void*)begin, uint64(end - begin) * sizeof(ElemType));  // NOLINT(bugprone-sizeof-expression)
	}

	template<
		typename ElemType,
		SOUL_REQUIRE(!is_trivially_move_constructible_v<ElemType>)
	>
	void Move(ElemType* begin, ElemType* end, ElemType* dst) {
		for (ElemType* iter = begin; iter != end; ++iter, ++dst) {
			new (dst) ElemType(std::move(*iter));
		}
	}

	template<
		typename ElemType,
		SOUL_REQUIRE(is_trivially_destructible_v<ElemType>)
	>
	void Destruct(ElemType* begin, ElemType* end) {}

	template<
		typename ElemType,
		SOUL_REQUIRE(!is_trivially_destructible_v<ElemType>)
	>
	void Destruct(ElemType* begin, ElemType* end) {
		for (ElemType* iter = begin; iter != end; ++iter) {
			iter->~ElemType();
		}
	}

	
	template<
		typename PointerDst,
		typename PointerSrc,
		SOUL_REQUIRE(is_pointer_v<PointerDst>),
		SOUL_REQUIRE(is_pointer_v<PointerSrc>)
	>
	PointerDst Cast(PointerSrc srcPtr)
	{
		using Dst = std::remove_pointer_t<PointerDst>;
		if constexpr (!is_same_v<PointerDst, void*>) {
			SOUL_ASSERT(0, uintptr(srcPtr) % alignof(Dst) == 0, "Source pointer is not aligned in PointerDst alignment!");
		}
		return (PointerDst)srcPtr;
	}

	template<
		typename IntegralDst,
		typename IntegralSrc,
		SOUL_REQUIRE(is_integral_v<IntegralDst>),
		SOUL_REQUIRE(is_integral_v<IntegralSrc>)
	>
	IntegralDst Cast(IntegralSrc src) {
		SOUL_ASSERT(0, src <= std::numeric_limits<IntegralDst>::max(), "Source value is larger than the destintation type maximum!");
		SOUL_ASSERT(0, src >= std::numeric_limits<IntegralDst>::min(), "Source value is smaller than the destination type minimum!");
		return IntegralDst(src);
	}


}

