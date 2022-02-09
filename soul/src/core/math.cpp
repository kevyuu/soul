#include <algorithm>
#include <cmath>
#include <limits>

#include "core/math.h"
#include "core/dev_util.h"


namespace soul {

	float min(float f1, float f2) {
		return f1 < f2 ? f1 : f2;
	}

	float max(float f1, float f2) {
		return f1 > f2 ? f1 : f2;
	}

	Vec3f min(Vec3f v1, Vec3f v2) {
		return { min(v1.x, v2.x), min(v1.y, v2.y), min(v1.z, v2.z) };
	}

	Vec3f max(Vec3f v1, Vec3f v2) {
		return { max(v1.x, v2.x), max(v1.y, v2.y), max(v1.z, v2.z) };
	}

	uint64 floorLog2(uint64 val) {
		uint32 level = 0;
		while (val >>= 1) ++level;
		return level;
	}

	Quaternionf quaternionFromVec3f(const Vec3f& source, const Vec3f& destination) {
		Quaternionf res;
		Vec3f src = unit(source);
		Vec3f dst = unit(destination);
		float dotRes = dot(src, dst);
		if (dotRes >= 1.0f) {
			return quaternionIdentity();
		}
		if (dotRes <= -1.0f) {
			SOUL_NOT_IMPLEMENTED();
			return quaternionIdentity();
		}
		Vec3f xyz = cross(src, dst);
		res.x = xyz.x;
		res.y = xyz.y;
		res.z = xyz.z;
		res.w = sqrt(squareLength(src) * squareLength(dst)) + dotRes;
		return unit(res);
	}

	Quaternionf quaternionFromMat4(const Mat4f& mat) {
		Quaternionf quat;

		const float trace = mat.elem[0][0] + mat.elem[1][1] + mat.elem[2][2];

		if (trace > 0) {
			float s = sqrt(trace + 1);
			quat.w = 0.5f * s;
			s = 0.5f / s;
			quat.x = (mat.elem[2][1] - mat.elem[1][2]) * s;
			quat.y = (mat.elem[0][2] - mat.elem[2][0]) * s;
			quat.z = (mat.elem[1][0] - mat.elem[0][1]) * s;
		}
		else {

			// Find the index of the greatest diagonal
			size_t i = 0;
			if (mat.elem[1][1] > mat.elem[0][0]) { i = 1; }
			if (mat.elem[2][2] > mat.elem[i][i]) { i = 2; }

			// Get the next indices: (n+1)%3
			static constexpr size_t next_ijk[3] = { 1, 2, 0 };
			size_t j = next_ijk[i];
			size_t k = next_ijk[j];
			float s = sqrt((mat.elem[i][i] - (mat.elem[j][j] + mat.elem[k][k])) + 1);
			quat.mem[i] = 0.5f * s;
			if (s != 0.0f) {
				s = 0.5f / s;
			}
			quat.w = (mat.elem[k][j] - mat.elem[j][k]) * s;
			quat.mem[j] = (mat.elem[j][i] + mat.elem[i][j]) * s;
			quat.mem[k] = (mat.elem[k][i] + mat.elem[i][k]) * s;
		}
		return quat;
	}

	Quaternionf qtangent(Vec3f tbn[3], uint64 storageSize) {
		Vec4f columns[4] = { Vec4f(tbn[0], 0.0f), Vec4f(cross(tbn[2], tbn[0]), 0.0f), Vec4f(tbn[2], 0.0f), Vec4f(0, 0, 0, 1.0f) };
	
		Quaternionf q = quaternionFromMat4(mat4FromColumns(columns));
		q = unit(q);
		q = q.w > 0 ? q : q * -1;

		// Ensure w is never 0.0
		// Bias is 2^(nb_bits - 1) - 1
		const float bias = 1.0f / float((1 << (storageSize * CHAR_BIT - 1)) - 1);
		if (q.w < bias) {
			q.w = bias;

			const float factor = float(std::sqrt(1.0 - double(bias) * double(bias)));
			q.xyz *= factor;
		}

		// If there's a reflection ((n x t) . b <= 0), make sure w is negative
		if (dot(cross(tbn[0], tbn[2]), tbn[1]) < 0.0f) {
			q = q * -1;
		}

		return q;
	}

	Quaternionf quaternionIdentity()
	{
		return { 0.0f, 0.0f, 0.0f, 1.0f };
	}

	bool operator==(const Quaternionf& lhs, const Quaternionf& rhs) {
		return (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z) && (lhs.w == rhs.w);
	}

	bool operator!=(const Quaternionf& lhs, const Quaternionf& rhs) {
		return (lhs.x != rhs.x) || (lhs.y != rhs.y) || (lhs.z != rhs.z) || (lhs.w != rhs.w);
	}

	Quaternionf operator+(const Quaternionf& lhs, const Quaternionf& rhs) {
		return Quaternionf(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
	}

	Mat3f mat3Identity()
	{
		Mat3f res;
		res.elem[0][0] = 1.0f;
		res.elem[1][1] = 1.0f;
		res.elem[2][2] = 1.0f;
		return res;
	}
	
	Mat3f mat3Transpose(const Mat3f& matrix) {
		Mat3f res;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				res.elem[j][i] = matrix.elem[i][j];
			}
		}

		return res;
	}

	Mat3f mat3UpperLeft(const Mat4f& matrix)
	{
		Mat3f res;

		res.elem[0][0] = matrix.elem[0][0];
		res.elem[0][1] = matrix.elem[0][1];
		res.elem[0][2] = matrix.elem[0][2];

		res.elem[1][0] = matrix.elem[1][0];
		res.elem[1][1] = matrix.elem[1][1];
		res.elem[1][2] = matrix.elem[1][2];

		res.elem[2][0] = matrix.elem[2][0];
		res.elem[2][1] = matrix.elem[2][1];
		res.elem[2][2] = matrix.elem[2][2];

		return res;
	}

	Mat3f mat3Inverse(const Mat3f& x)
	{
		Mat3f inverted;

		const float a = x.elem[0][0];
		const float b = x.elem[0][1];
		const float c = x.elem[0][2];
		const float d = x.elem[1][0];
		const float e = x.elem[1][1];
		const float f = x.elem[1][2];
		const float g = x.elem[2][0];
		const float h = x.elem[2][1];
		const float i = x.elem[2][2];

		// Do the full analytic inverse
		const float A = e * i - f * h;
		const float B = f * g - d * i;
		const float C = d * h - e * g;
		inverted.elem[0][0] = A;                 // A
		inverted.elem[0][1] = c * h - b * i;     // D
		inverted.elem[0][2] = b * f - c * e;     // G
		inverted.elem[1][0] = B;                 // B
		inverted.elem[1][1] = a * i - c * g;     // E
		inverted.elem[1][2] = c * d - a * f;     // H
		inverted.elem[2][0] = C;                 // C
		inverted.elem[2][2] = b * g - a * h;     // F
		inverted.elem[2][2] = a * e - b * d;     // I

		const float det(a * A + b * B + c * C);
		for (soul_size row = 0; row < 3; ++row) {
			for (soul_size col = 0; col < 3; ++col) {
				inverted.elem[row][col] /= det;
			}
		}

		return inverted;
	}

	Mat3f operator*(const Mat3f& lhs, const Mat3f& rhs) {
		Mat3f res;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				res.elem[i][j] = 0;
				for (int k = 0; k < 3; k++) {
					res.elem[i][j] += lhs.elem[i][k] * rhs.elem[k][j];
				}
			}
		}
		return res;	
	}


	Vec3f operator*(const Mat3f& lhs, const Vec3f& rhs) {
		return Vec3f(dot(lhs.rows[0], rhs), dot(lhs.rows[1], rhs), dot(lhs.rows[2], rhs));
	}

	void operator*=(Mat3f& lhs, const Mat3f& rhs)
	{
		Mat3f res = lhs * rhs;
		lhs = res;
	}

	Mat3f cofactor(const Mat3f& m) {
		Mat3f cof;

		const float a = m.elem[0][0];
		const float b = m.elem[1][0];
		const float c = m.elem[2][0];
		const float d = m.elem[0][1];
		const float e = m.elem[1][1];
		const float f = m.elem[2][1];
		const float g = m.elem[0][2];
		const float h = m.elem[1][2];
		const float i = m.elem[2][2];

		cof.elem[0][0] = e * i - f * h;  // A
		cof.elem[0][1] = c * h - b * i;  // D
		cof.elem[0][2] = b * f - c * e;  // G
		cof.elem[1][0] = f * g - d * i;  // B
		cof.elem[1][1] = a * i - c * g;  // E
		cof.elem[1][2] = c * d - a * f;  // H
		cof.elem[2][0] = d * h - e * g;  // C
		cof.elem[2][1] = b * g - a * h;  // F
		cof.elem[2][2] = a * e - b * d;  // I

		return cof;
	}

	float determinant(const Mat3f& m)
	{
		return (m.elem[0][0] * (m.elem[1][1] * m.elem[2][2] - m.elem[1][2] * m.elem[2][1])
			- m.elem[0][1] * (m.elem[1][0] * m.elem[2][2] - m.elem[1][2] * m.elem[2][0])
			+ m.elem[0][2] * (m.elem[1][0] * m.elem[2][1] - m.elem[1][1] * m.elem[2][0]));
	}

	Quaternionf operator*(const Quaternionf& lhs, float scalar) {
		return { lhs.x * scalar, lhs.y * scalar, lhs.z * scalar, lhs.w * scalar };
	}

	Quaternionf operator/(const Quaternionf& lhs, float scalar) {
		return { lhs.x / scalar, lhs.y / scalar, lhs.z / scalar, lhs.w / scalar };
	}

	Quaternionf operator*(const Quaternionf& lhs, const Quaternionf& rhs) {

		Quaternionf result;
		result.x = lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y;
		result.y = lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x;
		result.z = lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w;
		result.w = lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y + lhs.z * rhs.z;

		return result;

	}

	Vec3f rotate(const Quaternionf& lhs, const Vec3f& rhs) {
		return (lhs.xyz * 2.0f * (dot(lhs.xyz, rhs))) +
			(rhs * (lhs.w * lhs.w - dot(lhs.xyz, lhs.xyz))) +
			cross(lhs.xyz, rhs) * 2.0f * lhs.w;
	}

	Quaternionf unit(const Quaternionf& quaternion) {
		return quaternion / length(quaternion);
	}

	float length(const Quaternionf& quaternion) {
		return sqrt(quaternion.x * quaternion.x + quaternion.y * quaternion.y + quaternion.z * quaternion.z + quaternion.w * quaternion.w);
	}

	float squareLength(const Quaternionf& quaternion) {
		return quaternion.x * quaternion.x + quaternion.y * quaternion.y + quaternion.z * quaternion.z + quaternion.w * quaternion.w;
	}

	float dot(const Quaternionf& q1, const Quaternionf& q2) {
		return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
	}

	Quaternionf slerp(const Quaternionf& q1, const Quaternionf& q2, float t) {
		float cosTheta = dot(q1, q2);
		float absCosTheta = std::abs(cosTheta);
		static constexpr float eps = 10.0f * std::numeric_limits<float>::epsilon();
		if ((1.0f - absCosTheta) < eps) {
			return unit(lerp(cosTheta < 0 ? q1 * -1 : q1, q2, t));
		}
		float lenq1q2 = std::sqrt(dot(q1, q1) * dot(q2, q2));
		float theta = std::acos(std::clamp(cosTheta / lenq1q2, -1.0f, 1.0f));
		float theta1 = theta * (1 - t);
		float theta2 = theta * t;
		float sinTheta = std::sin(theta);
		if (sinTheta < eps) {
			return unit(lerp(q1, q2, t));
		}
		float invSinTheta = 1 / sinTheta;
		float fac1 = std::sin(theta1) * invSinTheta;
		float fac2 = std::sin(theta2) * invSinTheta;
		Quaternionf tmp = unit(fac1 * q1 + (cosTheta < 0 ? -fac2 : fac2) * q2);
		SOUL_ASSERT(0, std::none_of(tmp.mem, tmp.mem + 4, [](float i) { return std::isnan(i);  }), "");
		return tmp;
	}

	Quaternionf lerp(const Quaternionf& q1, const Quaternionf& q2, float t) {
		return ((1 - t) * q1) + (t * q2);
	}

	Mat4f mat4Identity() {
		Mat4f res;
		res.elem[0][0] = 1.0f;
		res.elem[1][1] = 1.0f;
		res.elem[2][2] = 1.0f;
		res.elem[3][3] = 1.0f;
		return res;
	}

	Mat4f mat4Scale(Vec3f scale)
	{
		Mat4f res;
		res.elem[0][0] = scale.x;
		res.elem[1][1] = scale.y;
		res.elem[2][2] = scale.z;
		res.elem[3][3] = 1;
		return res;
	}

	Mat4f mat4Translate(Vec3f offset) {
		Mat4f res;
		for (int i = 0; i < 4; i++) {
			res.elem[i][i] = 1;
		}
		res.elem[0][3] = offset.x;
		res.elem[1][3] = offset.y;
		res.elem[2][3] = offset.z;
		return res;
	}

	Mat4f mat4Rotate(Vec3f axis, float angle)
	{
		Mat4f res;

		float cosVal = cos(angle);
		float invCosVal = 1 - cosVal;
		float sinVal = sin(angle);

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

	Mat4f mat4Rotate(const Mat4f& mat4)
	{
		Mat4f result;
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

	Mat4f mat4(const float* data) {
		Mat4f res;
		for (int i = 0; i < 16; i++) {
			res.mem[i] = data[i];
		}
		return res;
	}

	Mat4f mat4FromMat3(const Mat3f& srcMat)
	{
		Mat4f mat;
		mat.rows[0] = Vec4f(srcMat.rows[0], 0.0f);
		mat.rows[1] = Vec4f(srcMat.rows[1], 0.0f);
		mat.rows[2] = Vec4f(srcMat.rows[2], 0.0f);
		mat.rows[3] = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
		return mat;
	}

	Mat4f mat4FromColumns(Vec4f columns[4]) {
		Mat4f mat4;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				mat4.elem[i][j] = columns[j].mem[i];
			}
		}
		return mat4;
	}

	Mat4f mat4FromRows(Vec4f rows[4])
	{
		Mat4f mat4;
		mat4.rows[0] = rows[0];
		mat4.rows[1] = rows[1];
		mat4.rows[2] = rows[2];
		mat4.rows[3] = rows[3];
		return mat4;
	}

	Mat4f mat4Quaternion(const Quaternionf& quaternion) {

		Mat4f mat4;
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

	Mat4f mat4Transform(const Transformf& transform) {
		Mat4f mat4;
		const Quaternionf& quaternion = transform.rotation;
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

		mat4.elem[0][0] = (1 - 2 * s * (qyy + qzz)) * transform.scale.x;
		mat4.elem[0][1] = (2 * s * (qxy - qzw)) * transform.scale.y;
		mat4.elem[0][2] = (2 * s * (qxz + qyw)) * transform.scale.z;
		mat4.elem[0][3] = transform.position.x;

		mat4.elem[1][0] = (2 * s * (qxy + qzw)) * transform.scale.x;
		mat4.elem[1][1] = (1 - 2 * s * (qxx + qzz)) * transform.scale.y;
		mat4.elem[1][2] = (2 * s * (qyz - qxw)) * transform.scale.z;
		mat4.elem[1][3] = transform.position.y;

		mat4.elem[2][0] = (2 * s * (qxz - qyw)) * transform.scale.x;
		mat4.elem[2][1] = (2 * s * (qyz + qxw)) * transform.scale.y;
		mat4.elem[2][2] = (1 - 2 * s * (qxx + qyy)) * transform.scale.z;
		mat4.elem[2][3] = transform.position.z;

		mat4.elem[3][0] = 0;
		mat4.elem[3][1] = 0;
		mat4.elem[3][2] = 0;
		mat4.elem[3][3] = 1;

		return mat4;
	}

	Mat4f mat4View(Vec3f position, Vec3f target, Vec3f up) {
		Mat4f res;

		Vec3f direction = unit(target - position);
		Vec3f zAxis = -direction;
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

		Mat4f translateMat = mat4Translate(-position);
		res = res * translateMat;

		return res;
	}

	Mat4f mat4Perspective(float fov, float aspect, float zNear, float zFar) {
		Mat4f res;
		res.elem[0][0] = 1 / (aspect * tan(fov / 2));
		res.elem[1][1] = 1 / (tan(fov / 2));
		res.elem[2][2] = (zNear + zFar) * -1 / (zFar - zNear);
		res.elem[2][3] = (-2 * zFar * zNear) / (zFar - zNear);
		res.elem[3][2] = -1;
		res.elem[3][3] = 0;
		return res;
	}

	Mat4f mat4Perspective(const Mat4f& baseMat, float zNear, float zFar)
	{
		Mat4f res = baseMat;
		res.elem[2][2] = (zNear + zFar) * -1 / (zFar - zNear);
		res.elem[2][3] = (-2 * zFar * zNear) / (zFar - zNear);
		return res;
	}

	Mat4f mat4Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
	{
		Mat4f res;
		res.elem[0][0] = 2 / (right - left); 
		res.elem[1][1] = 2 / (top - bottom);
		res.elem[2][2] = -2 / (zFar - zNear);
		res.elem[3][3] = 1;

		res.elem[0][3] = -(right + left) / (right - left);
		res.elem[1][3] = -(top + bottom) / (top - bottom);
		res.elem[2][3] = -(zFar + zNear) / (zFar - zNear);

		return res;
	}

	Mat4f mat4Transpose(const Mat4f & matrix)
	{
		Mat4f res;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				res.elem[j][i] = matrix.elem[i][j];
			}
		}

		return res;
	}

	Mat4f mat4Inverse(const Mat4f & matrix)
	{
		Mat4f res;
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

		if (det == 0.0f)
			return Mat4f();

		det = 1.f / det;

		for (int i = 0; i < 16; ++i)
			res.mem[i] = res.mem[i] * det;

		return res;
	}

	Mat4f mat4RigidTransformInverse(const Mat4f& matrix)
	{
		Mat3f rt = mat3Transpose(mat3UpperLeft(matrix));
		const Vec3f t = rt * matrix.columns(3).xyz;
		Mat4f res;
		res.rows[0] = Vec4f(rt.rows[0], t.x);
		res.rows[1] = Vec4f(rt.rows[1], t.y);
		res.rows[2] = Vec4f(rt.rows[2], t.z);
		res.rows[3] = Vec4f(0, 0, 0, 1.0f);
		return res;
	}

	Vec3f project(const Mat4f& mat, const Vec3f& vec)
	{
		Vec4f res = mat * Vec4f(vec, 1.0f);
		return res.xyz / res.w;
	}

	Mat4f operator+(const Mat4f & lhs, const Mat4f & rhs)
	{
		Mat4f res;

		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				res.elem[i][j] = lhs.elem[i][j] + rhs.elem[i][j];
			}
		}

		return res;
	}

	Mat4f operator-(const Mat4f& lhs, const Mat4f& rhs) {
		Mat4f res;

		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				res.elem[i][j] = lhs.elem[i][j] - rhs.elem[i][j];
			}
		}

		return res;
	}

	Mat4f operator*(const Mat4f& lhs, const Mat4f& rhs) 
	{
		Mat4f res;

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

	Vec3f operator*(const Mat4f & lhs, const Vec3f & rhs)
	{
		float x = lhs.elem[0][0] * rhs.x + lhs.elem[0][1] * rhs.y + lhs.elem[0][2] * rhs.z + lhs.elem[0][3];
		float y = lhs.elem[1][0] * rhs.x + lhs.elem[1][1] * rhs.y + lhs.elem[1][2] * rhs.z + lhs.elem[1][3];
		float z = lhs.elem[2][0] * rhs.x + lhs.elem[2][1] * rhs.y + lhs.elem[2][2] * rhs.z + lhs.elem[2][3];
		return Vec3f(x, y, z);
	}

	Vec4f operator*(const Mat4f & lhs, const Vec4f & rhs)
	{
		float x = lhs.elem[0][0] * rhs.x + lhs.elem[0][1] * rhs.y + lhs.elem[0][2] * rhs.z + lhs.elem[0][3] * rhs.w;
		float y = lhs.elem[1][0] * rhs.x + lhs.elem[1][1] * rhs.y + lhs.elem[1][2] * rhs.z + lhs.elem[1][3] * rhs.w;
		float z = lhs.elem[2][0] * rhs.x + lhs.elem[2][1] * rhs.y + lhs.elem[2][2] * rhs.z + lhs.elem[2][3] * rhs.w;
		float w = lhs.elem[3][0] * rhs.x + lhs.elem[3][1] * rhs.y + lhs.elem[3][2] * rhs.z + lhs.elem[3][3] * rhs.w;
		return Vec4f(x, y, z, w);
	}

	void operator+=(Mat4f& lhs, const Mat4f& rhs)
	{
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				lhs.elem[i][j] += rhs.elem[i][j];
			}
		}
	}

	void operator-=(Mat4f& lhs, const Mat4f& rhs) {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				lhs.elem[i][j] -= rhs.elem[i][j];
			}
		}
	}

	void operator*=(Mat4f & lhs, const Mat4f & rhs)
	{
		Mat4f res = lhs * rhs;
		lhs = res;
	}

	Mat3f mat3FromMat4UpperLeft(const Mat4f& mat4) {
		Mat3f res;
		res.rows[0] = { mat4.elem[0][0], mat4.elem[0][1], mat4.elem[0][2] };
		res.rows[1] = { mat4.elem[1][0], mat4.elem[1][1], mat4.elem[1][2] };
		res.rows[2] = { mat4.elem[2][0], mat4.elem[2][1], mat4.elem[2][2] };
		return res;
	}

	AABB aabbCombine(AABB aabb1, AABB aabb2) {
		return AABB(min(aabb1.min, aabb2.min), max(aabb1.max, aabb2.max));
	}

	AABB aabbTransform(AABB aabb, const Mat4f& transform) {
		Vec3f translation = Vec3f(transform.elem[0][3], transform.elem[1][3], transform.elem[2][3]);
		AABB result = { translation, translation };
		for (size_t col = 0; col < 3; ++col) {
			for (size_t row = 0; row < 3; ++row) {
				const float a = transform.elem[row][col] * aabb.min.mem[col];
				const float b = transform.elem[row][col] * aabb.max.mem[col];
				result.min.mem[row] += a < b ? a : b;
				result.max.mem[row] += a < b ? b : a;
			}
		}
		return result;
	}

	Transformf transformIdentity() {
		Transformf result;
		result.position = Vec3f(0.0f, 0.0f, 0.0f);
		result.rotation = quaternionIdentity();
		result.scale = Vec3f(1.0f, 1.0f, 1.0f);
		return result;
	}

	Transformf transformMat4(const Mat4f& mat4) {
		const float* elem = mat4.mem;
		Transformf result;
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
			float w4 = sqrt(1.0f + tr) * 2.0f;
			result.rotation.w = 0.25f * w4;
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

	bool operator==(const Transformf& lhs, const Transformf& rhs) {
		return (lhs.position == rhs.position) && (lhs.rotation == rhs.rotation) && (lhs.scale == rhs.scale);
	}

	bool operator!=(const Transformf& lhs, const Transformf& rhs) {
		return (lhs.position != rhs.position) || (lhs.rotation != rhs.rotation) || (lhs.scale != rhs.scale);
	}

	Transformf operator*(const Transformf& lhs, const Transformf& rhs)
	{
		return transformMat4(mat4Transform(lhs) * mat4Transform(rhs));
	}

	Vec3f operator*(const Transformf& lhs, const Vec3f& rhs)
	{
		return mat4Transform(lhs) * rhs;
	}

	float radians(float angle) {
		return (angle / 180) * Fconst::PI;
	}

	uint64 roundToNextPowOfTwo(uint64 num) {
		
		num -= 1;
		num |= (num >> 1);
		num |= (num >> 2);
		num |= (num >> 4);
		num |= (num >> 8);
		num |= (num >> 16);

		return num + 1;
	}

	uint32 hashMurmur32(const uint8* data, soul_size size) {
		uint32 h = 0;
		if (size > 3) {
			auto keyX4 = (const uint32*)data;
			size_t i = size >> 2;
			do {
				uint32 k = *keyX4++;
				k *= 0xcc9e2d51;
				k = (k << 15) | (k >> 17);
				k *= 0x1b873593;
				h ^= k;
				h = (h << 13) | (h >> 19);
				h = (h * 5) + 0xe6546b64;
			} while (--i);
			data = (const uint8*)keyX4;
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