#pragma once

#include "type.h"

namespace Soul {

	static const real32 PI = 3.14f;

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
	Vec3f unit(const Vec3f& vec);
	real32 length(const Vec3f& vec);

	Mat4 mat4Identity();
	Mat4 mat4Scale(Vec3f scale);
	Mat4 mat4Translate(Vec3f position);
	Mat4 mat4Rotate(Vec3f axis, real32 angle);
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

	real32 radians(real32 angle);

	int nextPowerOfTwo(int num);

	uint32 hashMurmur32(const char* key, uint32 keyLength);

	
};