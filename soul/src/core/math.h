#pragma once

#include "type.h"

namespace Soul {

	static const real32 PI = 3.14f;

	float min(float f1, float f2);
	float max(float f1, float f2);

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
	Vec3f cross(const Vec3f& lhs, const Vec3f& rhs);
	real32 dot(const Vec3f& lhs, const Vec3f& rhs);
	Vec3f componentMul(const Vec3f& lhs, const Vec3f& rhs);
	Vec3f unit(const Vec3f& vec);
	real32 squareLength(const Vec3f& vec);
	real32 length(const Vec3f& vec);
	Vec3f componentMin(const Vec3f& v1, const Vec3f& v2);
	Vec3f componentMax(const Vec3f& v1, const Vec3f& v2);

	Quaternion quaternionFromVec3f(const Vec3f& source, const Vec3f& dst);
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

	int roundToNextPowOfTwo(uint32 num);

	uint32 hashMurmur32(const char* key, uint32 keyLength);

};