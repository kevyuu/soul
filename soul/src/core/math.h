#pragma once

#include "type.h"
#include <cmath>
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

	static const real32 PI = 3.14f;

	real32 min(real32 f1, real32 f2);

	real32 max(real32 f1, real32 f2);

	uint32 floorLog2(uint32 val);
	

	Vec2f operator+(const Vec2f& lhs, const Vec2f& rhs);
	Vec2f operator-(const Vec2f& lhs, const Vec2f& rhs);
	Vec2f operator*(const Vec2f& lhs, const float rhs);
	void operator+=(Vec2f& lhs, const Vec2f& rhs);
	void operator-=(Vec2f& lhs, const Vec2f& rhs);
	void operator*=(Vec2f& lhs, const float rhs);

	Vec3f operator+(const Vec3f& lhs, const Vec3f& rhs);
	Vec3f operator-(const Vec3f& lhs, const Vec3f& rhs);
	Vec3f operator*(const Vec3f& lhs, const float rhs);
	Vec3f operator/(const Vec3f& lhs, const float rhs);
	void operator+=(Vec3f& lhs, const Vec3f& rhs);
	void operator-=(Vec3f& lhs, const Vec3f& rhs);
	void operator*=(Vec3f& lhs, const float rhs);
	void operator/=(Vec3f& lhs, const float rhs);
	bool operator==(const Vec3f& lhs, const Vec3f& rhs);
	bool operator!=(const Vec3f& lhs, const Vec3f& rhs);

	void operator/=(Vec4f& lhs, const float rhs);

	Vec3f cross(const Vec3f& lhs, const Vec3f& rhs);
	real32 dot(const Vec3f& lhs, const Vec3f& rhs);
	Vec3f componentMul(const Vec3f& lhs, const Vec3f& rhs);
	Vec3f unit(const Vec3f& vec);
	real32 squareLength(const Vec3f& vec);
	real32 length(const Vec3f& vec);
	Vec3f componentMin(const Vec3f& v1, const Vec3f& v2);
	Vec3f componentMax(const Vec3f& v1, const Vec3f& v2);

	Quaternion quaternionFromVec3f(const Vec3f& source, const Vec3f& dst);
	Quaternion quaternionFromMat4(const Mat4& mat);
	Quaternion qtangent(Vec3f tbn[3], uint64 storageSize = sizeof(int16));
	Quaternion quaternionIdentity();
	bool operator==(const Quaternion& lhs, const Quaternion& rhs);
	bool operator!=(const Quaternion& lhs, const Quaternion& rhs);
	Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs);
	Quaternion operator*(const Quaternion& lhs, float scalar);
	Quaternion operator/(const Quaternion& lhs, float scalar);
	void operator*=(const Quaternion& lhs, const Quaternion& rhs);
	Vec3f operator*(const Quaternion& lhs, const Vec3f& rhs);
	Quaternion unit(const Quaternion& quaternion);
	float length(const Quaternion& quaternion);
	float squareLength(const Quaternion& quaternion);
	Vec3f rotate(const Quaternion& quaternion, const Vec3f& vec);

	Mat4 mat4(const float* data);
	Mat4 mat4(const double* data);
	Mat4 mat4FromColumns(Vec4f columns[4]);
	Mat4 mat4Quaternion(const Quaternion& quaternion);
	Mat4 mat4Identity();
	Mat4 mat4Scale(Vec3f scale);
	Mat4 mat4Translate(Vec3f position);
	Mat4 mat4Rotate(Vec3f axis, real32 angle);
	Mat4 mat4Rotate(const Mat4& mat4);
	Mat4 mat4Transform(const Transform& transform);
	Mat4 mat4View(Vec3f position, Vec3f target, Vec3f up);
	Mat4 mat4Perspective(real32 fov, real32 aspectRatio, real32 near, real32 far);
	Mat4 mat4Ortho(real32 left, real32 right, real32 bottom, real32 top, real32 near, real32 far);
	Mat4 mat4Transpose(const Mat4& matrix);
	Mat4 mat4Inverse(const Mat4& matrix);
	Mat4 operator+(const Mat4& lhs, const Mat4& rhs);
	Mat4 operator-(const Mat4& lhs, const Mat4& rhs);
	Mat4 operator*(const Mat4& lhs, const Mat4& rhs);
	Vec3f operator* (const Mat4& lhs, const Vec3f& rhs);
	Vec4f operator* (const Mat4& lhs, const Vec4f& rhs);
	void operator+=(Mat4& lhs, const Mat4& rhs);
	void operator-=(Mat4& lhs, const Mat4& rhs);
	void operator*=(Mat4& lhs, const Mat4& rhs);
	bool operator==(const Mat4& lhs, const Mat4& rhs);
	bool operator!=(const Mat4& lhs, const Mat4& rhs);

	Transform transformIdentity();
	Transform transformMat4(const Mat4& mat4);
	bool operator==(const Transform& lhs, const Transform& rhs);
	bool operator!=(const Transform& lhs, const Transform& rhs);
	Transform operator*(const Transform& lhs, const Transform& rhs);
	void operator*=(Transform& lhs, const Transform& rhs);
	Vec3f operator*(const Transform& lhs, const Vec3f& rhs);

	real32 radians(real32 angle);

	inline bool isPowerOfTwo(uint32 num) { return (num & -num) == num;}
	int roundToNextPowOfTwo(uint32 num);

	uint32 hashMurmur32(const uint8* data, uint32 size);
	constexpr uint64 hashFNV1(const uint8* data, uint32 size, uint64 initial  = 0xcbf29ce484222325ull) {
		uint64 hash = initial;
		for (uint32 i = 0; i < size; i++) {
			hash = (hash * 0x100000001b3ull) ^ data[i];
		}
		return hash;
	}

};