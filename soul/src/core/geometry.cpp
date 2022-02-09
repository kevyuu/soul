#include <limits>

#include "core/geometry.h"
#include "core/math.h"
#include "runtime/scope_allocator.h"

namespace soul {

	Plane::Plane(const Vec3f& normal, const Vec3f& point) noexcept : normal(normal), d(dot(normal, point)) {};
	Plane::Plane(const Vec3f& normal, float d) noexcept : normal(normal), d(d) {};
	Plane::Plane(const Vec3f& p1, const Vec3f& p2, const Vec3f& p3)
	{
		normal = cross(p2 - p1, p3 - p1);
		d = dot(normal, p1);
	}

	Ray::Ray(const Vec3f& origin, const Vec3f& direction) noexcept : origin(origin), direction(direction) {};

	Frustum::Frustum(const Mat4f& mat)
	{
		// Note(kevinyu): Gribb/Hartmann method, reference: http://www.cs.otago.ac.nz/postgrads/alexis/planeExtraction.pdf

		Vec4f l = -mat.rows[3] - mat.rows[0];
		Vec4f r = -mat.rows[3] + mat.rows[0];
		Vec4f b = -mat.rows[3] - mat.rows[1];
		Vec4f t = -mat.rows[3] + mat.rows[1];
		Vec4f n = -mat.rows[3] - mat.rows[2];
		Vec4f f = -mat.rows[3] + mat.rows[2];

		l /= length(l.xyz);
		r /= length(r.xyz);
		b /= length(b.xyz);
		t /= length(t.xyz);
		n /= length(n.xyz);
		f /= length(f.xyz);

		planes[Side::LEFT] = Plane(l.xyz, -l.w);
		planes[Side::RIGHT] = Plane(r.xyz, -r.w);
		planes[Side::BOTTOM] = Plane(b.xyz, -b.w);
		planes[Side::TOP] = Plane(t.xyz, -t.w);
		planes[Side::NEAR] = Plane(n.xyz, -n.w);
		planes[Side::FAR] = Plane(f.xyz, -f.w);

	}

	Frustum::Frustum(const Vec3f corners[8])
	{
		Vec3f a = corners[0];
		Vec3f b = corners[1];
		Vec3f c = corners[2];
		Vec3f d = corners[3];
		Vec3f e = corners[4];
		Vec3f f = corners[5];
		Vec3f g = corners[6];
		Vec3f h = corners[7];

		//     c----d
		//    /|   /|
		//   g----h |
		//   | a--|-b
		//   |/   |/
		//   e----f

		planes[Side::LEFT] = Plane(a, e, g);
		planes[Side::RIGHT] = Plane(f, b, d);
		planes[Side::BOTTOM] = Plane(a, b, f);
		planes[Side::TOP] = Plane(g, h, d);
		planes[Side::FAR] = Plane(a, c, d);
		planes[Side::NEAR] = Plane(e, f, h);
	}

	bool FrustumCull(const Frustum& frustum, const Vec3f& center, const Vec3f& halfExtent)
	{
		for (auto& plane : frustum.planes)
		{
			const float dot =
				plane.normal.x * center.x - std::abs(plane.normal.x) * halfExtent.x +
				plane.normal.y * center.y - std::abs(plane.normal.y) * halfExtent.y +
				plane.normal.z * center.z - std::abs(plane.normal.z) * halfExtent.z -
				plane.d;
			if (!signbit(dot)) return false;
		}
		return true;
	}

	bool FrustumCull(const Frustum& frustum, const Vec4f& sphere)
	{
		for (auto& plane : frustum.planes)
		{
			const float dot =
				plane.normal.x * sphere.x +
				plane.normal.y * sphere.y +
				plane.normal.z * sphere.z -
				plane.d - sphere.w;
			if (!signbit(dot)) return false;
		}
		return true;
	}

	static Vec3f RandomPerp(const Vec3f& n) {
		Vec3f perp = cross(n, Vec3f( 1, 0, 0 ));
		float sqrlen = dot(perp, perp);
		if (sqrlen <= std::numeric_limits<float>::epsilon()) {
			perp = cross(n, Vec3f(0, 1, 0));
			sqrlen = dot(perp, perp);
		}
		return perp / sqrlen;
	}

	IntersectPointResult IntersectRayPlane(const Ray& ray, const Plane& plane) {
		if (dot(plane.normal, ray.direction) == 0.0f) {
			return { Vec3f(), false };
		}

		const float t = (plane.d - dot(plane.normal, ray.origin)) / (dot(plane.normal, ray.direction));
		if (t < 0) {
			return { Vec3f(), false };
		}
		
		return { ray.origin + t * ray.direction, true };

	}

	IntersectPointResult IntersectSegmentQuad(Vec3f s1, Vec3f s2, Vec3f q1, Vec3f q2, Vec3f q3, Vec3f q4) {
		IntersectPointResult res = IntersectSegmentTriangle(s1, s2, q1, q2, q3);
		if (res.intersect) return res;
		return IntersectSegmentTriangle(s1, s2, q1, q3, q4);
	}

	IntersectPointResult IntersectSegmentTriangle(Vec3f s1, Vec3f s2, Vec3f t1, Vec3f t2, Vec3f t3)
	{
		// Moller-Trumbore algorithm
		// read https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
		// We use the notation from the page above except "T", we use s1t1 in this code

		Vec3f e1 = t2 - t1;
		Vec3f e2 = t3 - t1;
		Vec3f d = s2 - s1;
		Vec3f p = cross(d, e2);
		float det = dot(e1, p);

		constexpr const float EPSILON = 1.0f / 65536.0f;  // ~1e-5
		if (std::abs(det) < EPSILON)
		{
			return { {}, false };
		}

		// s1t1 is T in the reference page, we need to use lower case t later
		Vec3f s1t1 = s1 - t1;
		Vec3f q = cross(s1t1, e1);

		// Cramer's rule to get the barycentric coordinate of the point in triangle
		// Here we calculate barycentric coordinate before divided by determinant,
		// so when doing the point in triangle test, we check against abs(det) instead of 1, reduce expensive division computation
		float u = dot(s1t1, p) * sign(det);
		float v = dot(d, q) * sign(det);
		if (u < 0 || v <  0 || u + v > std::abs(det)) return { {}, false };

		float t = dot(e2, q) * sign(det);
		if (t < 0 || t > std::abs(det)) return { {}, false };

		Vec3f result = s1 + d * (t / std::abs(det));
		return { result, true };
	}
		
	static bool ComputeTangentFrameWithTangents(const TangentFrameComputeInput& input, Quaternionf* qtangents) {
		const uint64 vertexCount = input.vertexCount;

		for (soul_size i = 0; i < vertexCount; ++i) {
			Vec3f normal = input.normals[i];
			auto tangent = Vec3f(input.tangents[i].xyz);
			const float tandir = input.tangents[i].w;
			Vec3f bitangent = tandir > 0 ? cross(tangent, normal) : cross(normal, tangent);
			
			//  Fix tangent in case tangent is not orthogonal to normal
			tangent = tandir > 0 ? cross(normal, bitangent) : cross(bitangent, normal);
			Vec3f tbn[3] = { tangent, bitangent, normal };
			qtangents[i] = qtangent(tbn);

		}
		return true;
	}

	static bool ComputeTangentFrameWithNormalsOnly(const TangentFrameComputeInput& input, Quaternionf* qtangents) {

		for (soul_size i = 0; i < input.vertexCount; ++i) {
			Vec3f normal = input.normals[i];
			Vec3f bitangent = RandomPerp(normal);
			Vec3f tbn[3] = { cross(normal, bitangent), bitangent, normal };
			qtangents[i] = qtangent(tbn);
		}

		return true;
	}

	static bool ComputeTangentFrameWithFlatNormals(const TangentFrameComputeInput& input, Quaternionf* qtangents) {
		runtime::ScopeAllocator<> scopeAllocator("ComputeTangentWithFlatNormals");
		Array<Vec3f> normals(&scopeAllocator);
		normals.resize(input.triangleCount);
		const uint64 vertexCount = input.vertexCount;
		const Vec3f* positions = input.positions;
		for (size_t a = 0; a < input.triangleCount; ++a) {
			const Vec3ui32 tri = input.triangles32[a];
			SOUL_ASSERT(0, tri.x < vertexCount && tri.y < vertexCount&& tri.z < vertexCount, "");
			const Vec3f v1 = positions[tri.x];
			const Vec3f v2 = positions[tri.y];
			const Vec3f v3 = positions[tri.z];
			const Vec3f normal = unit(cross(v2 - v1, v3 - v1));
			normals[tri.x] = normal;
			normals[tri.y] = normal;
			normals[tri.z] = normal;
		}

		TangentFrameComputeInput input2 = input;
		input2.normals = normals.data();
		ComputeTangentFrameWithNormalsOnly(input2, qtangents);
		return true;
	}

	static bool ComputeTangentFrameWithUVs(const TangentFrameComputeInput& input, Quaternionf* qtangents) {
		runtime::ScopeAllocator<> scopeAllocator("ComputeTangentWithUVs");
		Array<Vec3f> tan1(&scopeAllocator);
		tan1.resize(input.vertexCount);
		Array<Vec3f> tan2(&scopeAllocator);
		tan2.resize(input.vertexCount);
		
		static auto randomPerp = [](const Vec3f& n) -> Vec3f {
			Vec3f perp = cross(n, Vec3f{ 1, 0, 0 });
			float sqrlen = dot(perp, perp);
			if (sqrlen <= std::numeric_limits<float>::epsilon()) {
				perp = cross(n, Vec3f{ 0, 1, 0 });
				sqrlen = dot(perp, perp);
			}
			return perp / sqrlen;
		};


		for (soul_size a = 0; a < input.triangleCount; ++a) {
			Vec3ui32 tri = input.triangles32[a];
			SOUL_ASSERT(0, tri.x < input.vertexCount&& tri.y < input.vertexCount && tri.z < input.vertexCount, "");
			const Vec3f& v1 = input.positions[tri.x];
			const Vec3f& v2 = input.positions[tri.y];
			const Vec3f& v3 = input.positions[tri.z];
			const Vec2f& w1 = input.uvs[tri.x];
			const Vec2f& w2 = input.uvs[tri.y];
			const Vec2f& w3 = input.uvs[tri.z];
			float x1 = v2.x - v1.x;
			float x2 = v3.x - v1.x;
			float y1 = v2.y - v1.y;
			float y2 = v3.y - v1.y;
			float z1 = v2.z - v1.z;
			float z2 = v3.z - v1.z;
			float s1 = w2.x - w1.x;
			float s2 = w3.x - w1.x;
			float t1 = w2.y - w1.y;
			float t2 = w3.y - w1.y;
			float d = s1 * t2 - s2 * t1;
			Vec3f sdir, tdir;
			// In general we can't guarantee smooth tangents when the UV's are non-smooth, but let's at
			// least avoid divide-by-zero and fall back to normals-only method.
			if (d == 0.0f) {
				const Vec3f& n1 = input.normals[tri.x];
				sdir = randomPerp(n1);
				tdir = cross(n1, sdir);
			}
			else {
				sdir = { t2 * x1 - t1 * x2, t2 * y1 - t1 * y2, t2 * z1 - t1 * z2 };
				tdir = { s1 * x2 - s2 * x1, s1 * y2 - s2 * y1, s1 * z2 - s2 * z1 };
				float r = 1.0f / d;
				sdir *= r;
				tdir *= r;
			}
			tan1[tri.x] += sdir;
			tan1[tri.y] += sdir;
			tan1[tri.z] += sdir;
			tan2[tri.x] += tdir;
			tan2[tri.y] += tdir;
			tan2[tri.z] += tdir;
		}

		for (size_t a = 0; a < input.vertexCount; a++) {
			const Vec3f& n = input.normals[a];
			const Vec3f& t1 = tan1[a];
			const Vec3f& t2 = tan2[a];

			// Gram-Schmidt orthogonalize
			Vec3f t = unit(t1 - n * dot(n, t1));

			// Calculate handedness
			float w = (dot(cross(n, t1), t2) < 0.0f) ? -1.0f : 1.0f;

			Vec3f b = w < 0 ? cross(t, n) : cross(n, t);
			Vec3f tbn[3] = { t, b, n };
			Quaternionf q = qtangent(tbn);
			qtangents[a] = q;
		}
		return true;
	}

	bool ComputeTangentFrame(const TangentFrameComputeInput& input, Quaternionf* qtangents) {
		if (input.vertexCount == 0) {
			SOUL_LOG_ERROR("Vertex count cannot be zero");
			return false;
		}
		if (input.triangles32 != nullptr) {
			if (input.positions == nullptr) {
				SOUL_LOG_ERROR("Position is required");
				return false;
			}
			if (input.triangleCount == 0) {
				SOUL_LOG_ERROR("Triangle count is required");
				return false;
			}
			if (input.normals == nullptr) {
				return ComputeTangentFrameWithFlatNormals(input, qtangents);
			}
		}
		if (input.normals == nullptr) {
			SOUL_LOG_ERROR("Normals are required");
			return false;
		}
		if (input.tangents != nullptr) {
			return ComputeTangentFrameWithTangents(input, qtangents);
		}
		if (input.uvs == nullptr) {
			return ComputeTangentFrameWithNormalsOnly(input, qtangents);
		}
		return ComputeTangentFrameWithUVs(input, qtangents);


	}
}