#include "core/math.h"
#include "core/dev_util.h"

namespace Soul {

	real32 min(real32 f1, real32 f2) {
		return f1 < f2 ? f1 : f2;
	}

	real32 max(real32 f1, real32 f2) {
		return f1 > f2 ? f1 : f2;
	}

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

	Vec3f componentMul(const Vec3f& lhs, const Vec3f& rhs) {
		return {
			lhs.x * rhs.x,
			lhs.y * rhs.y,
			lhs.z * rhs.z
		};
	}

	Vec3f unit(const Vec3f & vec)
	{
		real32 magnitude = sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
		return Vec3f(vec.x / magnitude, vec.y / magnitude, vec.z / magnitude);
	}

	real32 squareLength(const Vec3f& vec) {
		return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
	}

	real32 length(const Vec3f & vec)
	{
		return sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
	}

	Vec3f componentMin(const Vec3f& v1, const Vec3f& v2) {
		return { min(v1.x, v2.x), min(v1.y, v2.y), min(v1.z, v2.z) };
	}

	Vec3f componentMax(const Vec3f& v1, const Vec3f& v2) {
		return { max(v1.x, v2.x), max(v1.y, v2.y), max(v1.z, v2.z) };
	}

	Quaternion quaternionFromVec3f(const Vec3f& source, const Vec3f& destination) {
		Quaternion res;
		Vec3f src = unit(source);
		Vec3f dst = unit(destination);
		float dotRes = dot(src, dst);
		if (dotRes >= 1.0f) {
			return quaternionIdentity();
		}
		if (dotRes <= -1.0f) {
			SOUL_ASSERT(0, false, "Have not been implemented yet");
			return quaternionIdentity();
		}
		Vec3f xyz = cross(src, dst);
		res.x = xyz.x;
		res.y = xyz.y;
		res.z = xyz.z;
		res.w = sqrt(squareLength(src) * squareLength(dst)) + dotRes;
		return unit(res);
	}

	Quaternion quaternionIdentity()
	{
		return { 0.0f, 0.0f, 0.0f, 1.0f };
	}

	bool operator==(const Quaternion& lhs, const Quaternion& rhs) {
		return (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z) && (lhs.w == rhs.w);
	}

	bool operator!=(const Quaternion& lhs, const Quaternion& rhs) {
		return (lhs.x != rhs.x) || (lhs.y != rhs.y) || (lhs.z != rhs.z) || (lhs.w != rhs.w);
	}

	Quaternion operator*(const Quaternion& lhs, float scalar) {
		return { lhs.x * scalar, lhs.y * scalar, lhs.z * scalar, lhs.w * scalar };
	}

	Quaternion operator/(const Quaternion& lhs, float scalar) {
		return { lhs.x / scalar, lhs.y / scalar, lhs.z / scalar, lhs.w / scalar };
	}

	Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs) {

		Quaternion result;
		result.x = lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y;
		result.y = lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x;
		result.z = lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w;
		result.w = lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y + lhs.z * rhs.z;

		return result;

	}

	Vec3f operator*(const Quaternion& lhs, const Vec3f& rhs) {
		
		return (lhs.xyz() * 2 * (dot(lhs.xyz(), rhs))) +
			(rhs * (lhs.w * lhs.w - dot(lhs.xyz(), lhs.xyz()))) +
			cross(lhs.xyz(), rhs) * 2 * lhs.w;
	
	}

	Quaternion unit(const Quaternion& quaternion) {
		return quaternion / length(quaternion);
	}

	float length(const Quaternion& quaternion) {
		return sqrt(quaternion.x * quaternion.x + quaternion.y * quaternion.y + quaternion.z * quaternion.z + quaternion.w * quaternion.w);
	}

	float squareLength(const Quaternion& quaternion) {
		return quaternion.x * quaternion.x + quaternion.y * quaternion.y + quaternion.z * quaternion.z + quaternion.w * quaternion.w;
	}

	Vec3f rotate(const Quaternion& quaternion, const Vec3f& vec3) {
		//TODO: make faster
		return mat4Quaternion(quaternion) * vec3;
	}

	Mat4 mat4Identity() {
		Mat4 res;
		res.elem[0][0] = 1.0f;
		res.elem[1][1] = 1.0f;
		res.elem[2][2] = 1.0f;
		res.elem[3][3] = 1.0f;
		return res;
	}

	Mat4 mat4Scale(Vec3f scale)
	{
		Mat4 res;
		res.elem[0][0] = scale.x;
		res.elem[1][1] = scale.y;
		res.elem[2][2] = scale.z;
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

	Mat4 mat4Rotate(const Mat4& mat4)
	{
		Mat4 result;
		for (int i = 0; i < 16; i++)
		{
			result.mem[i] = 0.0f;
		}

		const float* elem = mat4.mem;
		Vec3f scale = Vec3f(
			length(Vec3f(elem[0], elem[4], elem[8])),
			length(Vec3f(elem[1], elem[5], elem[9])),
			length(Vec3f(elem[2], elem[6], elem[10]))
		);

		result.mem[0] = elem[0] / scale.x;
		result.mem[4] = elem[4] / scale.x;
		result.mem[8] = elem[8] / scale.x;

		result.mem[1] = elem[1] / scale.y;
		result.mem[5] = elem[5] / scale.y;
		result.mem[9] = elem[9] / scale.y;

		result.mem[2] = elem[2] / scale.z;
		result.mem[6] = elem[6] / scale.z;
		result.mem[10] = elem[10] / scale.z;
	
		result.mem[15] = 1.0f;

		return result;
	}

	Mat4 mat4(const float* data) {
		Mat4 res;
		for (int i = 0; i < 16; i++) {
			res.mem[i] = data[i];
		}
		return res;
	}

	Mat4 mat4(const double* data) {
		Mat4 res;
		for (int i = 0; i < 16; i++) {
			res.mem[i] = data[i];
		}
		return res;
	}

	Mat4 mat4Quaternion(const Quaternion& quaternion) {

		Mat4 mat4;
		float s = squareLength(quaternion);
		
		float qxx = quaternion.x * quaternion.x;
		float qxy = quaternion.x * quaternion.y;
		float qxz = quaternion.x * quaternion.z;
		float qxw = quaternion.x * quaternion.w;

		float qyy = quaternion.y * quaternion.y;
		float qyz = quaternion.y * quaternion.z;
		float qyw = quaternion.y * quaternion.w;

		float qzz = quaternion.z * quaternion.z;
		float qzw = quaternion.z * quaternion.w;

		mat4.elem[0][0] = 1 - 2 * s * (qyy + qzz);
		mat4.elem[0][1] = 2 * s * (qxy - qzw);
		mat4.elem[0][2] = 2 * s * (qxz + qyw);
		mat4.elem[0][3] = 0;
		
		mat4.elem[1][0] = 2 * s * (qxy + qzw);
		mat4.elem[1][1] = 1 - 2 * s * (qxx + qzz);
		mat4.elem[1][2] = 2 * s * (qyz - qxw);
		mat4.elem[1][3] = 0;
		
		mat4.elem[2][0] = 2 * s * (qxz - qyw);
		mat4.elem[2][1] = 2 * s * (qyz + qxw);
		mat4.elem[2][2] = 1 - 2 * s * (qxx + qyy);
		mat4.elem[2][3] = 0;

		mat4.elem[3][0] = 0;
		mat4.elem[3][1] = 0;
		mat4.elem[3][2] = 0;
		mat4.elem[3][3] = 1;

		return mat4;

	}

	Mat4 mat4Transform(const Transform& transform) {
		// TODO: make more efficient by not using matrix multiplication. Instead compose the matrix manually itself
		return mat4Translate(transform.position) * mat4Quaternion(transform.rotation) * mat4Scale(transform.scale);
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

	bool operator==(const Mat4& lhs, const Mat4& rhs) {
		for (int i = 0; i < 16; i++) {
			if (lhs.mem[i] != rhs.mem[i]) return false;
		}
		return true;
	}

	bool operator!=(const Mat4& lhs, const Mat4& rhs) {
		for (int i = 0; i < 16; i++) {
			if (lhs.mem[i] != rhs.mem[i]) return true;
		}
		return false;
	}

	Transform transformIdentity() {
		Transform result;
		result.position = Vec3f(0.0f, 0.0f, 0.0f);
		result.rotation = quaternionIdentity();
		result.scale = Vec3f(1.0f, 1.0f, 1.0f);
		return result;
	}

	Transform transformMat4(const Mat4& mat4) {
		const float* elem = mat4.mem;
		Transform result;
		result.position = Vec3f(elem[3], elem[7], elem[11]);
		result.scale = Vec3f(
			length(Vec3f(elem[0], elem[4], elem[8])),
			length(Vec3f(elem[1], elem[5], elem[9])),
			length(Vec3f(elem[2], elem[6], elem[10]))
		);

		float elem0 = elem[0] / result.scale.x;
		float elem4 = elem[4] / result.scale.x;
		float elem8 = elem[8] / result.scale.x;

		float elem1 = elem[1] / result.scale.y;
		float elem5 = elem[5] / result.scale.y;
		float elem9 = elem[9] / result.scale.y;

		float elem2 = elem[2] / result.scale.z;
		float elem6 = elem[6] / result.scale.z;
		float elem10 = elem[10] / result.scale.z;

		float tr = elem0 + elem5 + elem10;
		if (tr > 0) {
			float w4 = sqrt(1.0 + tr) * 2.0f;
			result.rotation.w = 0.25 * w4;
			result.rotation.x = (elem9 - elem6) / w4;
			result.rotation.y = (elem2 - elem8) / w4;
			result.rotation.z = (elem4 - elem1) / w4;
		}
		else if ((elem0 > elem5) && (elem0 > elem10)) {
			float S = sqrt(1 + elem0 - elem5 - elem10) * 2;
			result.rotation.w = (elem9 - elem6) / S;
			result.rotation.x = 0.25f * S;
			result.rotation.y = (elem1 + elem4) / S;
			result.rotation.z = (elem2 + elem8) / S;
		}
		else if (elem5 > elem10) {
			float S = sqrt(1.0f + elem5 - elem0 - elem10) * 2;
			result.rotation.w = (elem2 - elem8) / S;
			result.rotation.x = (elem1 + elem4) / S;
			result.rotation.y = 0.25f * S;
			result.rotation.z = (elem6 + elem9) / S;
		}
		else {
			float S = sqrt(1.0f + elem10 - elem0 - elem5) * 2;
			result.rotation.w = (elem4 - elem1) / S;
			result.rotation.x = (elem2 + elem8) / S;
			result.rotation.y = (elem6 + elem9) / S;
			result.rotation.z = 0.25f * S;
		}
		result.rotation = unit(result.rotation);
		return result;
	}

	bool operator==(const Transform& lhs, const Transform& rhs) {
		return (lhs.position == rhs.position) && (lhs.rotation == rhs.rotation) && (lhs.scale == rhs.scale);
	}

	bool operator!=(const Transform& lhs, const Transform& rhs) {
		return (lhs.position != rhs.position) || (lhs.rotation != rhs.rotation) || (lhs.scale != rhs.scale);
	}

	Transform operator*(const Transform& lhs, const Transform& rhs)
	{
		return transformMat4(mat4Transform(lhs) * mat4Transform(rhs));
	}

	Vec3f operator*(const Transform& lhs, const Vec3f& rhs)
	{
		return mat4Transform(lhs) * rhs;
	}

	real32 radians(real32 angle) {
		return (angle / 180) * PI;
	}

	int roundToNextPowOfTwo(uint32 num) {
		
		num -= 1;
		num |= (num >> 1);
		num |= (num >> 2);
		num |= (num >> 4);
		num |= (num >> 8);
		num |= (num >> 16);

		return num + 1;
	}

	uint32 hashMurmur32(const uint8* data, uint32 size) {
		uint32 h = 0;
		if (size > 3) {
			const uint32* key_x4 = (const uint32*)data;
			size_t i = size >> 2;
			do {
				uint32 k = *key_x4++;
				k *= 0xcc9e2d51;
				k = (k << 15) | (k >> 17);
				k *= 0x1b873593;
				h ^= k;
				h = (h << 13) | (h >> 19);
				h = (h * 5) + 0xe6546b64;
			} while (--i);
			data = (const uint8*)key_x4;
		}
		if (size & 3) {
			uint32 i = size & 3;
			uint32 k = 0;
			data = &data[i - 1];
			do {
				k <<= 8;
				k |= *data--;
			} while (--i);
			k *= 0xcc9e2d51;
			k = (k << 15) | (k >> 17);
			k *= 0x1b873593;
			h ^= k;
		}
		h ^= size;
		h ^= h >> 16;
		h *= 0x85ebca6b;
		h ^= h >> 13;
		h *= 0xc2b2ae35;
		h ^= h >> 16;
		return h;
	}

}