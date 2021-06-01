#pragma once

#include "type.h"

namespace Soul {

	namespace DCONST {
		constexpr const double E = 2.71828182845904523536028747135266250;
		constexpr const double LOG2E = 1.44269504088896340735992468100189214;
		constexpr const double LOG10E = 0.434294481903251827651128918916605082;
		constexpr const double LN2 = 0.693147180559945309417232121458176568;
		constexpr const double LN10 = 2.30258509299404568401799145468436421;
		constexpr const double PI = 3.14159265358979323846264338327950288;
		constexpr const double PI_2 = 1.57079632679489661923132169163975144;
		constexpr const double PI_4 = 0.785398163397448309615660845819875721;
		constexpr const double ONE_OVER_PI = 0.318309886183790671537767526745028724;
		constexpr const double TWO_OVER_PI = 0.636619772367581343075535053490057448;
		constexpr const double TWO_OVER_SQRTPI = 1.12837916709551257389615890312154517;
		constexpr const double SQRT2 = 1.41421356237309504880168872420969808;
		constexpr const double SQRT1_2 = 0.707106781186547524400844362104849039;
		constexpr const double TAU = 2.0 * DCONST::PI;
		constexpr const double DEG_TO_RAD = DCONST::PI / 180.0;
		constexpr const double RAD_TO_DEG = 180.0 / DCONST::PI;
	}

	namespace FCONST {
		constexpr const float E = (float)DCONST::E;
		constexpr const float LOG2E = (float)DCONST::LOG2E;
		constexpr const float LOG10E = (float)DCONST::LOG10E;
		constexpr const float LN2 = (float)DCONST::LN2;
		constexpr const float LN10 = (float)DCONST::LN10;
		constexpr const float PI = (float)DCONST::PI;
		constexpr const float PI_2 = (float)DCONST::PI_2;
		constexpr const float PI_4 = (float)DCONST::PI_4;
		constexpr const float ONE_OVER_PI = (float)DCONST::ONE_OVER_PI;
		constexpr const float TWO_OVER_PI = (float)DCONST::TWO_OVER_PI;
		constexpr const float TWO_OVER_SQRTPI = (float)DCONST::TWO_OVER_SQRTPI;
		constexpr const float SQRT2 = (float)DCONST::SQRT2;
		constexpr const float SQRT1_2 = (float)DCONST::SQRT1_2;
		constexpr const float TAU = (float)DCONST::TAU;
		constexpr const float DEG_TO_RAD = (float)DCONST::DEG_TO_RAD;
		constexpr const float RAD_TO_DEG = (float)DCONST::RAD_TO_DEG;
	}


	float clamp(float f, float min, float max);

	uint32 floorLog2(uint32 val);	

	Vec2f operator+(const Vec2f& lhs, const Vec2f& rhs);
	Vec2f operator-(const Vec2f& lhs, const Vec2f& rhs);
	Vec2f operator*(const Vec2f& lhs, const float rhs);
	inline Vec2f operator*(const float lhs, const Vec2f& rhs) { return rhs * lhs; }
	void operator+=(Vec2f& lhs, const Vec2f& rhs);
	void operator-=(Vec2f& lhs, const Vec2f& rhs);
	void operator*=(Vec2f& lhs, const float rhs);
	Vec2f unit(Vec2f unit);

	Vec3f operator+(const Vec3f& lhs, const Vec3f& rhs);
	Vec3f operator-(const Vec3f& lhs, const Vec3f& rhs);
	Vec3f operator*(const Vec3f& lhs, const float rhs);
	inline Vec3f operator*(float lhs, const Vec3f& rhs) { return rhs * lhs; }
	Vec3f operator/(const Vec3f& lhs, const float rhs);
	void operator+=(Vec3f& lhs, const Vec3f& rhs);
	void operator-=(Vec3f& lhs, const Vec3f& rhs);
	void operator*=(Vec3f& lhs, const float rhs);
	void operator/=(Vec3f& lhs, const float rhs);
	bool operator==(const Vec3f& lhs, const Vec3f& rhs);
	bool operator!=(const Vec3f& lhs, const Vec3f& rhs);	
	Vec3f cross(const Vec3f& lhs, const Vec3f& rhs);
	float dot(const Vec3f& lhs, const Vec3f& rhs);
	Vec3f componentMul(const Vec3f& lhs, const Vec3f& rhs);
	Vec3f unit(const Vec3f& vec);
	float squareLength(const Vec3f& vec);
	float length(const Vec3f& vec);
	Vec3f componentMin(const Vec3f& v1, const Vec3f& v2);
	Vec3f componentMax(const Vec3f& v1, const Vec3f& v2);

	Vec4f operator/(const Vec4f& lhs, const float rhs);
	void operator*=(Vec4f& lhs, const float rhs);
	void operator/=(Vec4f& lhs, const float rhs);
	float squareLength(const Vec4f& vec);
	float length(const Vec4f& vec);
	
	Quaternionf quaternionFromVec3f(const Vec3f& source, const Vec3f& dst);
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
	Mat4f mat4FromColumns(Vec4f columns[4]);
	Mat4f mat4Quaternion(const Quaternionf& quaternion);
	Mat4f mat4Identity();
	Mat4f mat4Scale(Vec3f scale);
	Mat4f mat4Translate(Vec3f position);
	Mat4f mat4Rotate(Vec3f axis, float angle);
	Mat4f mat4Rotate(const Mat4f& mat4);
	Mat4f mat4Transform(const Transformf& transform);
	Mat4f mat4View(Vec3f position, Vec3f target, Vec3f up);
	Mat4f mat4Perspective(float fov, float aspectRatio, float near, float far);
	Mat4f mat4Ortho(float left, float right, float bottom, float top, float near, float far);
	Mat4f mat4Transpose(const Mat4f& matrix);
	Mat4f mat4Inverse(const Mat4f& matrix);
	Mat4f operator+(const Mat4f& lhs, const Mat4f& rhs);
	Mat4f operator-(const Mat4f& lhs, const Mat4f& rhs);
	Mat4f operator*(const Mat4f& lhs, const Mat4f& rhs);
	Vec3f operator* (const Mat4f& lhs, const Vec3f& rhs);
	Vec4f operator* (const Mat4f& lhs, const Vec4f& rhs);
	void operator+=(Mat4f& lhs, const Mat4f& rhs);
	void operator-=(Mat4f& lhs, const Mat4f& rhs);
	void operator*=(Mat4f& lhs, const Mat4f& rhs);
	bool operator==(const Mat4f& lhs, const Mat4f& rhs);
	bool operator!=(const Mat4f& lhs, const Mat4f& rhs);

	Mat3f mat3Transpose(const Mat3f& matrix);
	Mat3f operator*(const Mat3f& lhs, const Mat3f& rhs);
	void operator*=(Mat3f& lhs, const Mat3f& rhs);
	Mat3f cofactor(const Mat3f& mat);

	AABB AABBCombine(AABB aabb1, AABB aabb2);
	AABB AABBTransform(AABB aabb, const Mat4f& transform);

	Transformf transformIdentity();
	Transformf transformMat4(const Mat4f& mat4);
	bool operator==(const Transformf& lhs, const Transformf& rhs);
	bool operator!=(const Transformf& lhs, const Transformf& rhs);
	Transformf operator*(const Transformf& lhs, const Transformf& rhs);
	void operator*=(Transformf& lhs, const Transformf& rhs);
	Vec3f operator*(const Transformf& lhs, const Vec3f& rhs);

	float radians(float angle);

	inline bool isPowerOfTwo(int64 num) { return (num & -num) == num;}
	int roundToNextPowOfTwo(uint32 num);

	uint32 hashMurmur32(const uint8* data, uint32 size);
	constexpr uint64 hashFNV1(const uint8* data, uint32 size, uint64 initial  = 0xcbf29ce484222325ull) {
		uint64 hash = initial;
		for (uint32 i = 0; i < size; i++) {
			hash = (hash * 0x100000001b3ull) ^ data[i];
		}
		return hash;
	}

	template <typename T>
	T cubicSpline(const T& vert0, const T& tang0, const T& vert1, const T& tang1, float t) {
		float tt = t * t, ttt = tt * t;
		float s2 = -2 * ttt + 3 * tt, s3 = ttt - tt;
		float s0 = 1 - s2, s1 = s3 - tt + t;
		T p0 = vert0;
		T m0 = tang0;
		T p1 = vert1;
		T m1 = tang1;
		return s0 * p0 + s1 * m0 * t + s2 * p1 + s3 * m1 * t;
	}

};