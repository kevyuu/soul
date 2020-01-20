//
// Created by Kevin Yudi Utama on 2/8/18.
//

#pragma once

#include <cstdint>
#include <cstddef>
#include <climits>
#include <cfloat>
#include <type_traits>

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

typedef float real32;
typedef double real64;

typedef int8 s8;
typedef int8 s08;
typedef int16 s16;
typedef int32 s32;
typedef int64 s64;
typedef bool32 b32;

typedef uint8 u8;
typedef uint8 byte;
typedef uint8 u08;
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;

typedef real32 r32;
typedef real64 r64;
typedef real32 f32;
typedef real64 f64;

typedef uintptr_t umm;
typedef intptr_t smm;

typedef b32 b32x;
typedef u32 u32x;

namespace Soul {

	struct Vec2f;
	struct Vec3f;
	struct Vec4f;

	struct Vec2ui32 {
		uint32 x;
		uint32 y;

		Vec2ui32() : x(0), y(0) {}
		Vec2ui32(uint32 x, uint32 y) : x(x), y(y) {}
	};

	struct Vec2f {
		float x;
		float y;

		Vec2f() : x(0), y(0) {}
		Vec2f(float x, float y) : x(x), y(y) {}
	};

	struct Vec3f {
		float x;
		float y;
		float z;

		constexpr Vec3f() : x(0), y(0), z(0) {}
		constexpr Vec3f(float x, float y, float z) : x(x), y(y), z(z) {}
	};

	struct Vec4f {
		float x;
		float y;
		float z;
		float w;

		Vec4f() : x(0), y(0), z(0), w(0) {}
		Vec4f(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
		Vec4f(Vec3f xyz, float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
		Vec3f xyz() const { return Vec3f(x, y, z); }
	};

	struct Quaternion {
		float x;
		float y;
		float z;
		float w;

		Quaternion() : x(0), y(0), z(0), w(1) {}
		Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
		Quaternion(Vec3f xyz, float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
		Vec3f xyz() const { return Vec3f(x, y, z); }
	};

	struct Mat3 {

		float elem[3][3];

		Mat3() {
			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 3; j++) {
					elem[i][j] = 0;
				}
			}
		}
	};

	struct Mat4 {

		union {
			float elem[4][4];
			float mem[16];
		};


		Mat4() {
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					elem[i][j] = 0;
				}
			}
		}
	};

	struct Transform {
		Vec3f position;
		Vec3f scale;
		Quaternion rotation;
	};

	struct AABB {
		Vec3f min;
		Vec3f max;
	};

	template <typename ResourceType, typename IDType>
	struct ID {
		IDType id;

		constexpr ID() = default;
		constexpr explicit ID(IDType id) : id(id) {}
		bool operator==(const ID& other) const { return other.id == id; }
		bool operator!=(const ID& other) const { return other.id != id; }
		bool operator<(const ID& other) const { return id < other.id; }
		bool operator<=(const ID& other) const { return id <= other.id; }
	};

	template <typename Enum, typename T>
	void ForEachEnum(const T& func) {
		for (uint64 i = 0; i < uint64(Enum::COUNT); i++) {
			func(Enum(i));
		}
	}

	template<typename T>
	class Enum {

	public:
		class Iterator {
		public:
			Iterator(uint64 index): _index(index) {}
			Iterator operator++() { ++_index; return *this; }
			bool operator!=(const Iterator& other) const { return _index != other._index; }
			const T& operator*() const { return T(_index); }
		private:
			uint64 _index;
		};
		Iterator begin() const { return Iterator(0);}
		Iterator end() const { return Iterator(uint64(T::COUNT)); }

		static Enum Iterates() {
			return Enum();
		}

		static uint64 Count() {
			return uint64(T::COUNT);
		}

	private:
		Enum() = default;

	};

}

#define SOUL_ARRAY_LEN(arr) (sizeof(arr) / sizeof(*arr))
#define SOUL_BIT_COUNT(type) (sizeof(type) * 8)
#define SOUL_UTYPE_MAX(type) ((1u << (SOUL_BIT_COUNT(type) - 1)) - 1)
