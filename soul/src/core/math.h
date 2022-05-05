#pragma once

#include <algorithm>

#include "core/compiler.h"
#include "core/type.h"

namespace soul {

	namespace Dconst {
		constexpr double E = 2.71828182845904523536028747135266250;
		constexpr double LOG2_E = 1.44269504088896340735992468100189214;
		constexpr double LOG10_E = 0.434294481903251827651128918916605082;
		constexpr double LN2 = 0.693147180559945309417232121458176568;
		constexpr double LN10 = 2.30258509299404568401799145468436421;
		constexpr double PI = 3.14159265358979323846264338327950288;
		constexpr double PI_2 = 1.57079632679489661923132169163975144;
		constexpr double PI_4 = 0.785398163397448309615660845819875721;
		constexpr double ONE_OVER_PI = 0.318309886183790671537767526745028724;
		constexpr double TWO_OVER_PI = 0.636619772367581343075535053490057448;
		constexpr double TWO_OVER_SQRTPI = 1.12837916709551257389615890312154517;
		constexpr double SQRT2 = 1.41421356237309504880168872420969808;
		constexpr double SQRT1_2 = 0.707106781186547524400844362104849039;
		constexpr double TAU = 2.0 * Dconst::PI;
		constexpr double DEG_TO_RAD = Dconst::PI / 180.0;
		constexpr double RAD_TO_DEG = 180.0 / Dconst::PI;
	}

	namespace Fconst {
		constexpr float E = (float)Dconst::E;
		constexpr float LOG2_E = (float)Dconst::LOG2_E;
		constexpr float LOG10_E = (float)Dconst::LOG10_E;
		constexpr float LN2 = (float)Dconst::LN2;
		constexpr float LN10 = (float)Dconst::LN10;
		constexpr float PI = (float)Dconst::PI;
		constexpr float PI_2 = (float)Dconst::PI_2;
		constexpr float PI_4 = (float)Dconst::PI_4;
		constexpr float ONE_OVER_PI = (float)Dconst::ONE_OVER_PI;
		constexpr float TWO_OVER_PI = (float)Dconst::TWO_OVER_PI;
		constexpr float TWO_OVER_SQRTPI = (float)Dconst::TWO_OVER_SQRTPI;
		constexpr float SQRT2 = (float)Dconst::SQRT2;
		constexpr float SQRT1_2 = (float)Dconst::SQRT1_2;
		constexpr float TAU = (float)Dconst::TAU;
		constexpr float DEG_TO_RAD = (float)Dconst::DEG_TO_RAD;
		constexpr float RAD_TO_DEG = (float)Dconst::RAD_TO_DEG;
	}

	uint64 floorLog2(uint64 val);

	template<typename T>
	SOUL_NODISCARD constexpr Vec2<T> operator+(const Vec2<T>& lhs, const Vec2<T>& rhs) {
		return Vec2<T>(lhs.x + rhs.x, lhs.y + rhs.y);
	}

	template<typename T>
	SOUL_NODISCARD constexpr Vec2<T> operator-(const Vec2<T>& lhs, const Vec2<T>& rhs) {
		return Vec2<T>(lhs.x - rhs.x, lhs.y - rhs.y);
	}

	template<typename T>
	SOUL_NODISCARD constexpr Vec2<T> operator-(const Vec2<T>& vec)
	{
		return Vec2<T>(-vec.x, -vec.y);
	}

	template<typename T>
	SOUL_NODISCARD constexpr Vec2<T> operator*(const Vec2<T>& lhs, const Vec2<T>& rhs) {
		return Vec2<T>(lhs.x * rhs.x, lhs.y * rhs.y);
	}

	template<typename T>
	SOUL_NODISCARD constexpr Vec2<T> operator*(const Vec2<T>& lhs, T rhs) {
		return Vec2<T>(lhs.x * rhs, lhs.y * rhs);
	}

	template<typename T>
	SOUL_NODISCARD constexpr Vec2<T> operator*(T lhs, const Vec2<T>& rhs) {
		return Vec2<T>(lhs * rhs.x, lhs * rhs.y);
	}

	template<typename T>
	SOUL_NODISCARD constexpr Vec2<T> operator/(const Vec2<T>& lhs, const Vec2<T>& rhs) {
		return Vec2<T>(lhs.x / rhs.x, lhs.y / rhs.y);
	}

	template<typename T>
	SOUL_NODISCARD constexpr Vec2<T> operator/(const Vec2<T>& lhs, const T rhs) {
		return Vec2<T>(lhs.x / rhs, lhs.y / rhs);
	}

	template<typename T>
	SOUL_NODISCARD constexpr Vec2<T> operator/(const T lhs, const Vec2<T>& rhs)
	{
		return Vec2<T>(lhs / rhs.x, lhs / rhs.y);
	}

	template<typename T>
	void operator+=(Vec2<T>& lhs, const Vec2<T>& rhs) {
		lhs.x += rhs.x;
		lhs.y += rhs.y;
	}

	template<typename T>
	void operator-=(Vec2<T>& lhs, const Vec2<T>& rhs) {
		lhs.x -= rhs.x;
		lhs.y -= rhs.y;
	}

	template<typename T>
	void operator*=(Vec2<T>& lhs, const Vec2<T>& rhs) {
		lhs.x *= rhs.x;
		lhs.y *= rhs.y;
	}

	template<typename T>
	void operator*=(Vec2<T>& lhs, T rhs) {
		lhs.x *= rhs;
		lhs.y *= rhs;
	}

	template<typename T>
	void operator/=(Vec2<T>& lhs, const Vec2<T>& rhs) {
		lhs.x /= rhs.x;
		lhs.y /= rhs.y;
	}

	template<typename T>
	bool operator==(const Vec2<T>& lhs, const Vec2<T>& rhs) {
		return lhs.x == rhs.x && lhs.y == rhs.y;
	}

	template<typename T>
	bool operator!=(const Vec2<T>& lhs, const Vec2<T>& rhs) {
		return !(lhs == rhs);
	}

	template<typename T>
	SOUL_NODISCARD T squareLength(const Vec2<T>& vec) {
		return vec.x * vec.x + vec.y * vec.y;
	}

	template<typename T>
	SOUL_NODISCARD T length(const Vec2<T>& vec) {
		return std::sqrt(vec.x * vec.x + vec.y * vec.y);
	}

	template<typename T>
	SOUL_NODISCARD constexpr Vec2<T> unit(const Vec2<T>& vec) {
		return vec / length;
	}

	// Vec3
	template<typename T>
	SOUL_NODISCARD constexpr Vec3<T> operator+(const Vec3<T>& lhs, const Vec3<T>& rhs) {
		return Vec3<T>(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
	}

	template<typename T>
	SOUL_NODISCARD constexpr Vec3<T> operator-(const Vec3<T>& lhs, const Vec3<T>& rhs) {
		return Vec3<T>(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
	}

	template<typename T>
	SOUL_NODISCARD constexpr Vec3<T> operator-(const Vec3<T>& rhs) {
		return Vec3<T>(-rhs.x, -rhs.y, -rhs.z);
	}

	template<typename T>
	SOUL_NODISCARD Vec3<T> constexpr operator*(const Vec3<T>& lhs, T rhs) {
		return Vec3<T>(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
	}

	template<typename T>
	SOUL_NODISCARD Vec3<T> constexpr operator*(T lhs, const Vec3<T>& rhs) {
		return Vec3<T>(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z);
	}

	template<typename T>
	SOUL_NODISCARD constexpr Vec3<T> operator/(const Vec3<T>& lhs, T rhs) {
		return Vec3<T>(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
	}

	template<typename T>
	void operator+=(Vec3<T>& lhs, const Vec3<T>& rhs) {
		lhs.x += rhs.x;
		lhs.y += rhs.y;
		lhs.z += rhs.z;
	}

	template<typename T>
	void operator-=(Vec3<T>& lhs, const Vec3<T>& rhs) {
		lhs.x -= rhs.x;
		lhs.y -= rhs.y;
		lhs.z -= rhs.z;
	}

	template<typename T>
	void operator*=(Vec3<T>& lhs, T rhs) {
		lhs.x *= rhs;
		lhs.y *= rhs;
		lhs.z *= rhs;
	}

	template<typename T>
	void operator/=(Vec3<T>& lhs, T rhs) {
		lhs.x /= rhs;
		lhs.y /= rhs;
		lhs.z /= rhs;
	}

	template<typename T>
	bool operator==(const Vec3<T>& lhs, const Vec3<T>& rhs) {
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;  // NOLINT(clang-diagnostic-float-equal)
	}

	template<typename T>
	bool operator!=(const Vec3<T>& lhs, const Vec3<T>& rhs) {
		return !(lhs == rhs);
	}

	template<typename T>
	SOUL_NODISCARD Vec3<T> cross(const Vec3<T>& lhs, const Vec3<T>& rhs)
	{
		return Vec3<T>(
			lhs.y * rhs.z - lhs.z * rhs.y,
			lhs.z * rhs.x - lhs.x * rhs.z,
			lhs.x * rhs.y - lhs.y * rhs.x);
	}

	template<typename T>
	SOUL_NODISCARD float dot(const Vec3<T>& lhs, const Vec3<T>& rhs)
	{
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
	}

	template<typename T>
	SOUL_NODISCARD Vec3<T> unit(const Vec3<T>& vec)
	{
		float magnitude = sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
		return Vec3f(vec.x / magnitude, vec.y / magnitude, vec.z / magnitude);
	}

	template<typename T>
	SOUL_NODISCARD T squareLength(const Vec3<T>& vec)
	{
		return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
	}

	template<typename T>
	SOUL_NODISCARD T length(const Vec3<T>& vec)
	{
		return std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
	}

	template<typename T>
	SOUL_NODISCARD Vec3<T> componentMin(const Vec3<T>& v1, const Vec3<T>& v2)
	{
		return { std::min(v1.x, v2.x), std::min(v1.y, v2.y), std::min(v1.z, v2.z) };
	}

	template<typename T>
	SOUL_NODISCARD Vec3<T> componentMax(const Vec3<T>& v1, const Vec3<T>& v2)
	{
		return { std::max(v1.x, v2.x), std::max(v1.y, v2.y), std::max(v1.z, v2.z) };
	}

	// Vec4
	template<typename T>
	SOUL_NODISCARD constexpr Vec4<T> operator+(const Vec4<T>& lhs, const Vec4<T>& rhs) {
		return Vec4<T>(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
	}

	template<typename T>
	SOUL_NODISCARD constexpr Vec4<T> operator-(const Vec4<T>& lhs, const Vec4<T>& rhs) {
		return Vec4<T>(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
	}

	template<typename T>
	SOUL_NODISCARD constexpr Vec4<T> operator-(const Vec4<T>& rhs) {
		return Vec4<T>(-rhs.x, -rhs.y, -rhs.z, -rhs.w);
	}

	template<typename T>
	SOUL_NODISCARD Vec4<T> constexpr operator*(const Vec4<T>& lhs, T rhs) {
		return Vec4<T>(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs);
	}

	template<typename T>
	SOUL_NODISCARD Vec4<T> constexpr operator*(T lhs, const Vec4<T>& rhs) {
		return Vec4<T>(lhs * rhs.x, lhs * rhs.y, lhs * rhs.z, lhs * rhs.w);
	}

	template<typename T>
	SOUL_NODISCARD constexpr Vec4<T> operator/(const Vec4<T>& lhs, T rhs) {
		return Vec4<T>(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs);
	}

	template<typename T>
	void operator+=(Vec4<T>& lhs, const Vec4<T>& rhs) {
		lhs.x += rhs.x;
		lhs.y += rhs.y;
		lhs.z += rhs.z;
		lhs.w += rhs.w;
	}

	template<typename T>
	void operator-=(Vec4<T>& lhs, const Vec4<T>& rhs) {
		lhs.x -= rhs.x;
		lhs.y -= rhs.y;
		lhs.z -= rhs.z;
		lhs.w -= rhs.w;
	}

	template<typename T>
	void operator*=(Vec4<T>& lhs, T rhs) {
		lhs.x *= rhs;
		lhs.y *= rhs;
		lhs.z *= rhs;
		lhs.w *= rhs;
	}

	template<typename T>
	void operator/=(Vec4<T>& lhs, T rhs) {
		lhs.x /= rhs;
		lhs.y /= rhs;
		lhs.z /= rhs;
		lhs.w /= rhs;
	}

	template<typename T>
	bool operator==(const Vec4<T>& lhs, const Vec4<T>& rhs) {
		return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;  // NOLINT(clang-diagnostic-float-equal)
	}

	template<typename T>
	bool operator!=(const Vec4<T>& lhs, const Vec4<T>& rhs) {
		return !(lhs == rhs);
	}

	template<typename T>
	SOUL_NODISCARD float dot(const Vec4<T>& lhs, const Vec4<T>& rhs) {
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
	}

	template<typename T>
	SOUL_NODISCARD Vec4<T> unit(const Vec4<T>& vec) {
		float magnitude = sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z, + vec.w * vec.w);
		return Vec3f(vec.x / magnitude, vec.y / magnitude, vec.z / magnitude, vec.w / magnitude);
	}

	template<typename T>
	SOUL_NODISCARD T squareLength(const Vec4<T>& vec) {
		return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w;
	}

	template<typename T>
	SOUL_NODISCARD T length(const Vec4<T>& vec) {
		return std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w);
	}


	Quaternionf quaternionFromVec3f(const Vec3f& src, const Vec3f& dst);
	Quaternionf quaternionFromMat4(const Mat4f& mat);
	Quaternionf qtangent(Vec3f tbn[3], uint64 storageSize = sizeof(int16));
	Quaternionf quaternionIdentity();
	bool operator==(const Quaternionf& lhs, const Quaternionf& rhs);
	bool operator!=(const Quaternionf& lhs, const Quaternionf& rhs);
	Quaternionf operator+(const Quaternionf& lhs, const Quaternionf& rhs);
	Quaternionf operator*(const Quaternionf& lhs, const Quaternionf& rhs);
	Quaternionf operator*(const Quaternionf& lhs, float scalar);
	inline Quaternionf operator*(float scalar, const Quaternionf& q) { return q * scalar; }
	Quaternionf operator/(const Quaternionf& lhs, float scalar);
	Quaternionf unit(const Quaternionf& quaternion);
	float length(const Quaternionf& quaternion);
	float squareLength(const Quaternionf& quaternion);
	float dot(const Quaternionf& q1, const Quaternionf& q2);
	Vec3f rotate(const Quaternionf& quaternion, const Vec3f& vec);
	Quaternionf slerp(const Quaternionf& q1, const Quaternionf& q2, float t);
	Quaternionf lerp(const Quaternionf& q1, const Quaternionf& q2, float t);

	Mat4f mat4(const float* data);
	Mat4f mat4FromMat3(const Mat3f& mat3);
	Mat4f mat4FromColumns(Vec4f columns[4]);
	Mat4f mat4FromRows(Vec4f rows[4]);
	Mat4f mat4Quaternion(const Quaternionf& quaternion);
	Mat4f mat4Identity();
	Mat4f mat4Scale(Vec3f scale);
	Mat4f mat4Translate(Vec3f position);
	Mat4f mat4Rotate(Vec3f axis, float angle);
	Mat4f mat4Rotate(const Mat4f& mat4);
	Mat4f mat4Transform(const Transformf& transform);
	Mat4f mat4View(Vec3f position, Vec3f target, Vec3f up);
	Mat4f mat4Perspective(float fov, float aspectRatio, float near, float far);
	Mat4f mat4Perspective(const Mat4f& baseMat, float near, float far);
	Mat4f mat4Ortho(float left, float right, float bottom, float top, float near, float far);
	Mat4f mat4Transpose(const Mat4f& matrix);
	Mat4f mat4Inverse(const Mat4f& matrix);
	Mat4f mat4RigidTransformInverse(const Mat4f& matrix);
	Vec3f project(const Mat4f& mat, const Vec3f& vec);
	Mat4f operator+(const Mat4f& lhs, const Mat4f& rhs);
	Mat4f operator-(const Mat4f& lhs, const Mat4f& rhs);
	Mat4f operator*(const Mat4f& lhs, const Mat4f& rhs);
	Vec3f operator* (const Mat4f& lhs, const Vec3f& rhs);
	Vec4f operator* (const Mat4f& lhs, const Vec4f& rhs);
	void operator+=(Mat4f& lhs, const Mat4f& rhs);
	void operator-=(Mat4f& lhs, const Mat4f& rhs);
	void operator*=(Mat4f& lhs, const Mat4f& rhs);

	Mat3f mat3Identity();
	Mat3f mat3Transpose(const Mat3f& matrix);
	Mat3f mat3UpperLeft(const Mat4f& matrix);
	Mat3f mat3Inverse(const Mat3f& matrix);
	Mat3f operator*(const Mat3f& lhs, const Mat3f& rhs);
	Vec3f operator*(const Mat3f& lhs, const Vec3f& rhs);
	void operator*=(Mat3f& lhs, const Mat3f& rhs);
	Mat3f cofactor(const Mat3f& mat);
	float determinant(const Mat3f& mat);

	AABB aabbCombine(AABB aabb1, AABB aabb2);
	AABB aabbTransform(AABB aabb, const Mat4f& transform);

	Transformf transformIdentity();
	Transformf transformMat4(const Mat4f& mat4);
	bool operator==(const Transformf& lhs, const Transformf& rhs);
	bool operator!=(const Transformf& lhs, const Transformf& rhs);
	Transformf operator*(const Transformf& lhs, const Transformf& rhs);
	void operator*=(Transformf& lhs, const Transformf& rhs);
	Vec3f operator*(const Transformf& lhs, const Vec3f& rhs);

	float radians(float angle);

	inline bool isPowerOfTwo(int64 num) { return (num & -num) == num;}
	uint64 roundToNextPowOfTwo(uint64 num);

	uint32 hashMurmur32(const uint8* data, soul_size size);

	constexpr uint64 hash_fnv1_bytes(const uint8* data, soul_size size, uint64 initial  = 0xcbf29ce484222325ull) {
		uint64 hash = initial;
		for (uint32 i = 0; i < size; i++) {
			hash = (hash * 0x100000001b3ull) ^ data[i];
		}
		return hash;
	}

	template <typename T>
	constexpr uint64 hash_fnv1(const T* data, uint64 initial = 0xcbf29ce484222325ull)
	{
		return hash_fnv1_bytes(reinterpret_cast<const uint8*>(data), sizeof(data), initial);
	}

	template <typename T>
	T cubicSpline(const T& vert0, const T& tang0, const T vert1, const T& tang1, float t) {
		float tt = t * t, ttt = tt * t;
		float s2 = -2 * ttt + 3 * tt, s3 = ttt - tt;
		float s0 = 1 - s2, s1 = s3 - tt + t;
		T p0 = vert0;
		T m0 = tang0;
		T p1 = vert1;
		T m1 = tang1;
		return s0 * p0 + s1 * m0 * t + s2 * p1 + s3 * m1 * t;
	}

	template<typename T>
	inline constexpr T sign(T x) noexcept {
		return x < T(0) ? T(-1) : T(1);
	}

	inline int16_t packSnorm16(float v) noexcept {
		return static_cast<int16_t>(std::round(std::clamp(v, -1.0f, 1.0f) * 32767.0f));
	}

	inline Vec4i16 packSnorm16(Vec4f v) noexcept {
		return Vec4i16{ packSnorm16(v.x), packSnorm16(v.y), packSnorm16(v.z), packSnorm16(v.w) };
	}

};