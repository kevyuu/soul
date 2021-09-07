#pragma once

#include "core/type.h"

namespace Soul {

    // constant-normal form
    struct Plane {
        Vec3f normal;
        float d = 0;

        Plane(const Vec3f& normal, const Vec3f& point) noexcept;
        Plane(const Vec3f& normal, float d) noexcept;
    };

    struct Ray {
        Vec3f origin;
        Vec3f direction;
        Ray(const Vec3f& origin, const Vec3f& direction) noexcept;
    };

    struct IntersectPointResult
    {
        Vec3f point;
        bool intersect = false;
    };

    IntersectPointResult IntersectRayPlane(const Ray& ray, const Plane& plane);

    struct TangentFrameComputeInput {
        uint64 vertexCount = 0;
        const Vec3f* normals = nullptr;
        const Vec4f* tangents = nullptr;
        const Vec2f* uvs = nullptr;
        const Vec3f* positions = nullptr;
        const Vec3ui32* triangles32 = nullptr;
        uint64 triangleCount = 0;
        TangentFrameComputeInput(uint64 vertexCount, const Vec3f* normals, const Vec4f* tangents, const Vec2f* uvs, const Vec3f* positions, const Vec3ui32* triangles32, uint64 triangleCount)
	        :vertexCount(vertexCount), normals(normals), tangents(tangents), uvs(uvs), positions(positions), triangles32(triangles32), triangleCount(triangleCount) {
	        
        }
    };

    bool ComputeTangentFrame(const TangentFrameComputeInput& input, Quaternionf* qtangents);

}