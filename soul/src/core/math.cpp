#include "core/math.h"
#include <cmath>
#include <iostream>

namespace Soul {
	Vec2f operator+(const Vec2f & lhs, const Vec2f & rhs)
	{
		return Vec2f(lhs.x + rhs.x, lhs.y + rhs.y);
	}

	Vec2f operator-(const Vec2f & lhs, const Vec2f & rhs)
	{
		return Vec2f(lhs.x - rhs.x, lhs.y - rhs.y);
	}

	Vec2f operator*(const Vec2f & lhs, const float rhs)
	{
		return Vec2f(lhs.x * rhs, lhs.y * rhs);
	}

	void operator+=(Vec2f & lhs, const Vec2f & rhs)
	{
		lhs.x += rhs.x;
		lhs.y += rhs.y;
	}

	void operator-=(Vec2f & lhs, const Vec2f & rhs)
	{
		lhs.x -= rhs.x;
		lhs.y -= rhs.y;
	}

	void operator*=(Vec2f & lhs, const float rhs)
	{
		lhs.x *= rhs;
		lhs.y *= rhs;
	}

	Vec3f operator+(const Vec3f & lhs, const Vec3f & rhs)
	{
		return Vec3f(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
	}

	Vec3f operator-(const Vec3f & lhs, const Vec3f & rhs)
	{
		return Vec3f(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
	}

	Vec3f operator*(const Vec3f & lhs, const float rhs)
	{
		return Vec3f(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
	}

	Vec3f operator/(const Vec3f & lhs, const float rhs)
	{
		return Vec3f(lhs.x/rhs, lhs.y/rhs, lhs.z/rhs);
	}

	void operator+=(Vec3f & lhs, const Vec3f & rhs)
	{
		lhs.x += rhs.x;
		lhs.y += rhs.y;
		lhs.z += rhs.z;
	}

	void operator-=(Vec3f & lhs, const Vec3f & rhs)
	{
		lhs.x -= rhs.x;
		lhs.y -= rhs.y;
		lhs.z -= rhs.z;
	}

	void operator*=(Vec3f & lhs, const float rhs)
	{
		lhs.x *= rhs;
		lhs.y *= rhs;
		lhs.z *= rhs;
	}

	void operator/=(Vec3f & lhs, const float rhs)
	{
		lhs.x /= rhs;
		lhs.y /= rhs;
		lhs.z /= rhs;
	}

	bool operator==(const Vec3f& lhs, const Vec3f& rhs) {
		return (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z);
	}

	bool operator!=(const Vec3f& lhs, const Vec3f& rhs) {
		return (lhs.x != rhs.x) || (lhs.y != rhs.y) || (lhs.z != rhs.z);
	}

	Vec3f cross(const Vec3f & lhs, const Vec3f & rhs)
	{
		Vec3f res;

		res.x = lhs.y * rhs.z - lhs.z * rhs.y;
		res.y = lhs.z * rhs.x - lhs.x * rhs.z;
		res.z = lhs.x * rhs.y - lhs.y * rhs.x;

		return res;
	}

	real32 dot(const Vec3f & lhs, const Vec3f & rhs)
	{
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
	}

	Vec3f unit(const Vec3f & vec)
	{
		real32 magnitude = sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
		return Vec3f(vec.x / magnitude, vec.y / magnitude, vec.z / magnitude);
	}

	real32 length(const Vec3f & vec)
	{
		return sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
	}

	Mat4 mat4Identity() {
		Mat4 res;
		res.elem[0][0] = 1.0f;
		res.elem[1][1] = 1.0f;
		res.elem[2][2] = 1.0f;
		res.elem[3][3] = 1.0f;
		return res;
	}

	Mat4 mat4Scale(float scaleX, float scaleY, float scaleZ)
	{
		Mat4 res;
		res.elem[0][0] = scaleX;
		res.elem[1][1] = scaleY;
		res.elem[2][2] = scaleZ;
		res.elem[3][3] = 1;
		return res;
	}

	Mat4 mat4Translate(Vec3f offset) {
		Mat4 res;
		for (int i = 0; i < 4; i++) {
			res.elem[i][i] = 1;
		}
		res.elem[0][3] = offset.x;
		res.elem[1][3] = offset.y;
		res.elem[2][3] = offset.z;
		return res;
	}

	Mat4 mat4Rotate(Vec3f axis, real32 angle)
	{
		Mat4 res;

		real32 cosVal = cos(angle);
		real32 invCosVal = 1 - cosVal;
		real32 sinVal = sin(angle);

		res.elem[0][0] = cosVal + axis.x * axis.x * invCosVal;
		res.elem[0][1] = axis.x * axis.y * invCosVal - axis.z * sinVal;
		res.elem[0][2] = axis.x * axis.z * invCosVal + axis.y * sinVal;
		res.elem[1][0] = axis.y * axis.x * invCosVal + axis.z * sinVal;
		res.elem[1][1] = cosVal + axis.y * axis.y * invCosVal;
		res.elem[1][2] = axis.y * axis.z * invCosVal - axis.x * sinVal;
		res.elem[2][0] = axis.z * axis.x * invCosVal - axis.y * sinVal;
		res.elem[2][1] = axis.z * axis.y * invCosVal + axis.x * sinVal;
		res.elem[2][2] = cosVal + axis.z * axis.z * invCosVal;
		res.elem[3][3] = 1;

		return res;
	}

	Mat4 mat4View(Vec3f position, Vec3f target, Vec3f up) {
		Mat4 res;

		Vec3f direction = unit(target - position);
		Vec3f zAxis = direction * -1;
		Vec3f xAxis = unit(cross(direction , up));
		Vec3f yAxis = unit(cross(xAxis, direction));
		

		res.elem[0][0] = xAxis.x;
		res.elem[0][1] = xAxis.y;
		res.elem[0][2] = xAxis.z;

		res.elem[1][0] = yAxis.x;
		res.elem[1][1] = yAxis.y;
		res.elem[1][2] = yAxis.z;

		res.elem[2][0] = zAxis.x;
		res.elem[2][1] = zAxis.y;
		res.elem[2][2] = zAxis.z;

		res.elem[3][3] = 1;

		Mat4 translateMat = mat4Translate(position * -1);
		res = res * translateMat;

		return res;
	}

	Mat4 mat4Perspective(real32 fov, real32 aspect, real32 zNear, real32 zFar) {
		Mat4 res;
		res.elem[0][0] = 1 / (aspect * tan(fov / 2));
		res.elem[1][1] = 1 / (tan(fov / 2));
		res.elem[2][2] = (zNear + zFar) * -1 / (zFar - zNear);
		res.elem[2][3] = (-2 * zFar * zNear) / (zFar - zNear);
		res.elem[3][2] = -1;
		res.elem[3][3] = 0;
		return res;
	}

	Mat4 mat4Ortho(real32 left, real32 right, real32 bottom, real32 top, real32 zNear, real32 zFar)
	{
		Mat4 res;
		res.elem[0][0] = 2 / (right - left); 
		res.elem[1][1] = 2 / (top - bottom);
		res.elem[2][2] = -2 / (zFar - zNear);
		res.elem[3][3] = 1;

		res.elem[0][3] = -(right + left) / (right - left);
		res.elem[1][3] = -(top + bottom) / (top - bottom);
		res.elem[2][3] = -(zFar + zNear) / (zFar - zNear);

		return res;
	}

	Mat4 mat4Transpose(const Mat4 & matrix)
	{
		Mat4 res;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				res.elem[j][i] = matrix.elem[i][j];
			}
		}

		return res;
	}

	Mat4 mat4Inverse(const Mat4 & matrix)
	{
		Mat4 res;
		float det;
		
		res.mem[0] = matrix.mem[5] * matrix.mem[10] * matrix.mem[15] - matrix.mem[5] * matrix.mem[11] * matrix.mem[14] - matrix.mem[9] * matrix.mem[6] * matrix.mem[15] + matrix.mem[9] * matrix.mem[7] * matrix.mem[14] + matrix.mem[13] * matrix.mem[6] * matrix.mem[11] - matrix.mem[13] * matrix.mem[7] * matrix.mem[10];
		res.mem[4] = -matrix.mem[4] * matrix.mem[10] * matrix.mem[15] + matrix.mem[4] * matrix.mem[11] * matrix.mem[14] + matrix.mem[8] * matrix.mem[6] * matrix.mem[15] - matrix.mem[8] * matrix.mem[7] * matrix.mem[14] - matrix.mem[12] * matrix.mem[6] * matrix.mem[11] + matrix.mem[12] * matrix.mem[7] * matrix.mem[10];
		res.mem[8] = matrix.mem[4] * matrix.mem[9] * matrix.mem[15] - matrix.mem[4] * matrix.mem[11] * matrix.mem[13] - matrix.mem[8] * matrix.mem[5] * matrix.mem[15] + matrix.mem[8] * matrix.mem[7] * matrix.mem[13] + matrix.mem[12] * matrix.mem[5] * matrix.mem[11] - matrix.mem[12] * matrix.mem[7] * matrix.mem[9];
		res.mem[12] = -matrix.mem[4] * matrix.mem[9] * matrix.mem[14] + matrix.mem[4] * matrix.mem[10] * matrix.mem[13] + matrix.mem[8] * matrix.mem[5] * matrix.mem[14] - matrix.mem[8] * matrix.mem[6] * matrix.mem[13] - matrix.mem[12] * matrix.mem[5] * matrix.mem[10] + matrix.mem[12] * matrix.mem[6] * matrix.mem[9];
		res.mem[1] = -matrix.mem[1] * matrix.mem[10] * matrix.mem[15] + matrix.mem[1] * matrix.mem[11] * matrix.mem[14] + matrix.mem[9] * matrix.mem[2] * matrix.mem[15] - matrix.mem[9] * matrix.mem[3] * matrix.mem[14] - matrix.mem[13] * matrix.mem[2] * matrix.mem[11] + matrix.mem[13] * matrix.mem[3] * matrix.mem[10];
		res.mem[5] = matrix.mem[0] * matrix.mem[10] * matrix.mem[15] - matrix.mem[0] * matrix.mem[11] * matrix.mem[14] - matrix.mem[8] * matrix.mem[2] * matrix.mem[15] + matrix.mem[8] * matrix.mem[3] * matrix.mem[14] + matrix.mem[12] * matrix.mem[2] * matrix.mem[11] - matrix.mem[12] * matrix.mem[3] * matrix.mem[10];
		res.mem[9] = -matrix.mem[0] * matrix.mem[9] * matrix.mem[15] + matrix.mem[0] * matrix.mem[11] * matrix.mem[13] + matrix.mem[8] * matrix.mem[1] * matrix.mem[15] - matrix.mem[8] * matrix.mem[3] * matrix.mem[13] - matrix.mem[12] * matrix.mem[1] * matrix.mem[11] + matrix.mem[12] * matrix.mem[3] * matrix.mem[9];
		res.mem[13] = matrix.mem[0] * matrix.mem[9] * matrix.mem[14] - matrix.mem[0] * matrix.mem[10] * matrix.mem[13] - matrix.mem[8] * matrix.mem[1] * matrix.mem[14] + matrix.mem[8] * matrix.mem[2] * matrix.mem[13] + matrix.mem[12] * matrix.mem[1] * matrix.mem[10] - matrix.mem[12] * matrix.mem[2] * matrix.mem[9];
		res.mem[2] = matrix.mem[1] * matrix.mem[6] * matrix.mem[15] - matrix.mem[1] * matrix.mem[7] * matrix.mem[14] - matrix.mem[5] * matrix.mem[2] * matrix.mem[15] + matrix.mem[5] * matrix.mem[3] * matrix.mem[14] + matrix.mem[13] * matrix.mem[2] * matrix.mem[7] - matrix.mem[13] * matrix.mem[3] * matrix.mem[6];
		res.mem[6] = -matrix.mem[0] * matrix.mem[6] * matrix.mem[15] + matrix.mem[0] * matrix.mem[7] * matrix.mem[14] + matrix.mem[4] * matrix.mem[2] * matrix.mem[15] - matrix.mem[4] * matrix.mem[3] * matrix.mem[14] - matrix.mem[12] * matrix.mem[2] * matrix.mem[7] + matrix.mem[12] * matrix.mem[3] * matrix.mem[6];
		res.mem[10] = matrix.mem[0] * matrix.mem[5] * matrix.mem[15] - matrix.mem[0] * matrix.mem[7] * matrix.mem[13] - matrix.mem[4] * matrix.mem[1] * matrix.mem[15] + matrix.mem[4] * matrix.mem[3] * matrix.mem[13] + matrix.mem[12] * matrix.mem[1] * matrix.mem[7] - matrix.mem[12] * matrix.mem[3] * matrix.mem[5];
		res.mem[14] = -matrix.mem[0] * matrix.mem[5] * matrix.mem[14] + matrix.mem[0] * matrix.mem[6] * matrix.mem[13] + matrix.mem[4] * matrix.mem[1] * matrix.mem[14] - matrix.mem[4] * matrix.mem[2] * matrix.mem[13] - matrix.mem[12] * matrix.mem[1] * matrix.mem[6] + matrix.mem[12] * matrix.mem[2] * matrix.mem[5];
		res.mem[3] = -matrix.mem[1] * matrix.mem[6] * matrix.mem[11] + matrix.mem[1] * matrix.mem[7] * matrix.mem[10] + matrix.mem[5] * matrix.mem[2] * matrix.mem[11] - matrix.mem[5] * matrix.mem[3] * matrix.mem[10] - matrix.mem[9] * matrix.mem[2] * matrix.mem[7] + matrix.mem[9] * matrix.mem[3] * matrix.mem[6];
		res.mem[7] = matrix.mem[0] * matrix.mem[6] * matrix.mem[11] - matrix.mem[0] * matrix.mem[7] * matrix.mem[10] - matrix.mem[4] * matrix.mem[2] * matrix.mem[11] + matrix.mem[4] * matrix.mem[3] * matrix.mem[10] + matrix.mem[8] * matrix.mem[2] * matrix.mem[7] - matrix.mem[8] * matrix.mem[3] * matrix.mem[6];
		res.mem[11] = -matrix.mem[0] * matrix.mem[5] * matrix.mem[11] + matrix.mem[0] * matrix.mem[7] * matrix.mem[9] + matrix.mem[4] * matrix.mem[1] * matrix.mem[11] - matrix.mem[4] * matrix.mem[3] * matrix.mem[9] - matrix.mem[8] * matrix.mem[1] * matrix.mem[7] + matrix.mem[8] * matrix.mem[3] * matrix.mem[5];
		res.mem[15] = matrix.mem[0] * matrix.mem[5] * matrix.mem[10] - matrix.mem[0] * matrix.mem[6] * matrix.mem[9] - matrix.mem[4] * matrix.mem[1] * matrix.mem[10] + matrix.mem[4] * matrix.mem[2] * matrix.mem[9] + matrix.mem[8] * matrix.mem[1] * matrix.mem[6] - matrix.mem[8] * matrix.mem[2] * matrix.mem[5];

		det = matrix.mem[0] * res.mem[0] + matrix.mem[1] * res.mem[4] + matrix.mem[2] * res.mem[8] + matrix.mem[3] * res.mem[12];

		if (det == 0)
			return Mat4();

		det = 1.f / det;

		for (int i = 0; i < 16; i++)
			res.mem[i] = res.mem[i] * det;

		return res;
	}

	Mat4 operator+(const Mat4 & lhs, const Mat4 & rhs)
	{
		Mat4 res;

		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				res.elem[i][j] = lhs.elem[i][j] + rhs.elem[i][j];
			}
		}

		return res;
	}

	Mat4 operator-(const Mat4& lhs, const Mat4& rhs) {
		Mat4 res;

		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				res.elem[i][j] = lhs.elem[i][j] - rhs.elem[i][j];
			}
		}

		return res;
	}

	Mat4 operator*(const Mat4& lhs, const Mat4& rhs) 
	{
		Mat4 res;

		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				res.elem[i][j] = 0;
				for (int k = 0; k < 4; k++) {
					res.elem[i][j] += lhs.elem[i][k] * rhs.elem[k][j];
				}
			}
		}

		return res;
	}

	Vec3f operator*(const Mat4 & lhs, const Vec3f & rhs)
	{
		real32 x = lhs.elem[0][0] * rhs.x + lhs.elem[0][1] * rhs.y + lhs.elem[0][2] * rhs.z + lhs.elem[0][3];
		real32 y = lhs.elem[1][0] * rhs.x + lhs.elem[1][1] * rhs.y + lhs.elem[1][2] * rhs.z + lhs.elem[1][3];
		real32 z = lhs.elem[2][0] * rhs.x + lhs.elem[2][1] * rhs.y + lhs.elem[2][2] * rhs.z + lhs.elem[2][3];
		return Vec3f(x, y, z);
	}

	Vec4f operator*(const Mat4 & lhs, const Vec4f & rhs)
	{
		real32 x = lhs.elem[0][0] * rhs.x + lhs.elem[0][1] * rhs.y + lhs.elem[0][2] * rhs.z + lhs.elem[0][3] * rhs.w;
		real32 y = lhs.elem[1][0] * rhs.x + lhs.elem[1][1] * rhs.y + lhs.elem[1][2] * rhs.z + lhs.elem[1][3] * rhs.w;
		real32 z = lhs.elem[2][0] * rhs.x + lhs.elem[2][1] * rhs.y + lhs.elem[2][2] * rhs.z + lhs.elem[2][3] * rhs.w;
		real32 w = lhs.elem[3][0] * rhs.x + lhs.elem[3][1] * rhs.y + lhs.elem[3][2] * rhs.z + lhs.elem[3][3] * rhs.w;
		return Vec4f(x, y, z, w);
	}

	void operator+=(Mat4& lhs, const Mat4& rhs)
	{
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				lhs.elem[i][j] += rhs.elem[i][j];
			}
		}
	}

	void operator-=(Mat4& lhs, const Mat4& rhs) {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				lhs.elem[i][j] -= rhs.elem[i][j];
			}
		}
	}

	void operator*=(Mat4 & lhs, const Mat4 & rhs)
	{
		Mat4 res = lhs * rhs;
		lhs = res;
	}

	real32 radians(real32 angle) {
		return (angle / 180) * PI;
	}

	int nextPowerOfTwo(int num) {
		
		num -= 1;
		num |= (num >> 1);
		num |= (num >> 2);
		num |= (num >> 4);
		num |= (num >> 8);
		num |= (num >> 16);

		return num + 1;
	}
}