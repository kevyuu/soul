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
        Vec3f* normals = nullptr;
        Vec4f* tangents = nullptr;
        Vec2f* uvs = nullptr;
        Vec3f* positions = nullptr;
        Vec3ui32* triangles32 = nullptr;
        uint64 triangleCount = 0;
    };

    bool ComputeTangentFrame(const TangentFrameComputeInput& input, Quaternionf* qtangents);

}