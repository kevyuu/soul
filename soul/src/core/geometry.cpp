#include "core/geometry.h"
#include "core/math.h"
#include "memory/allocators/scope_allocator.h"

#include <limits>

namespace Soul {

	static Vec3f _RandomPerp(const Vec3f& n) {
		Vec3f perp = cross(n, Vec3f( 1, 0, 0 ));
		float sqrlen = dot(perp, perp);
		if (sqrlen <= std::numeric_limits<float>::epsilon()) {
			perp = cross(n, Vec3f(0, 1, 0));
			sqrlen = dot(perp, perp);
		}
		return perp / sqrlen;
	}

	static bool _ComputeTangentFrameWithTangents(const TangentFrameComputeInput& input, Quaternion* qtangents) {
		uint64 vertexCount = input.vertexCount;

		for (uint64 i = 0; i < vertexCount; i++) {
			Vec3f normal = input.normals[i];
			Vec3f tangent = Vec3f(input.tangents[i].xyz);
			const float tandir = input.tangents[i].w;
			Vec3f bitangent = tandir > 0 ? cross(tangent, normal) : cross(normal, tangent);
			
			//  Fix tangent in case tangent is not orthogonal to normal
			tangent = tandir > 0 ? cross(normal, bitangent) : cross(bitangent, normal);
			Vec3f tbn[3] = { tangent, bitangent, normal };
			qtangents[i] = qtangent(tbn);

		}

		return true;

	}

	static bool _ComputeTangentFrameWithNormalsOnly(const TangentFrameComputeInput& input, Quaternion* qtangents) {

		for (uint64 i = 0; i < input.vertexCount; i++) {
			Vec3f normal = input.normals[i];
			Vec3f bitangent = _RandomPerp(normal);
			Vec3f tbn[3] = { cross(normal, bitangent), bitangent, normal };
			qtangents[i] = qtangent(tbn);
		}

		return true;
	}

	static bool _ComputeTangentFrameWithFlatNormals(const TangentFrameComputeInput& input, Quaternion* qtangents) {
		Memory::ScopeAllocator<> scopeAllocator("ComputeTangentWithFlatNormals");
		Vec3f* normals = (Vec3f*) scopeAllocator.allocate(input.triangleCount * sizeof(Vec3f*), alignof(Vec3f), "").addr;
		uint64 vertexCount = input.vertexCount;
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
		input2.normals = normals;
		_ComputeTangentFrameWithNormalsOnly(input2, qtangents);
		return true;
	}

	static bool _ComputeTangentFrameWithUVs(const TangentFrameComputeInput& input, Quaternion* qtangents) {
		SOUL_NOT_IMPLEMENTED();
		return false;
	}
}

bool Soul::ComputeTangentFrame(const TangentFrameComputeInput& input, Quaternion* qtangents) {
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
			return _ComputeTangentFrameWithFlatNormals(input, qtangents);
		}
	}
	if (input.normals == nullptr) {
		SOUL_LOG_ERROR("Normals are required");
		return false;
	}
	if (input.tangents != nullptr) {
		return _ComputeTangentFrameWithTangents(input, qtangents);
	}
	if (input.uvs == nullptr) {
		return _ComputeTangentFrameWithNormalsOnly(input, qtangents);
	}
	return _ComputeTangentFrameWithUVs(input, qtangents);
}