#pragma once

#include "core/type.h"
#include "flag_map.h"

namespace soul {

    // constant-normal form, (ax + by + cz - d = 0), where (a, b, c) is normal and d is distance
    struct Plane {
        Vec3f normal;
        float d = 0;

        Plane() = default;
        Plane(const Vec3f& normal, const Vec3f& point) noexcept;
        Plane(const Vec3f& normal, float d) noexcept;
        Plane(const Vec3f& p1, const Vec3f& p2, const Vec3f& p3);
    };

    struct Frustum
    {

        enum class Side : uint8
        {
	        LEFT,
            RIGHT,
            BOTTOM,
            TOP,
            FAR,
            NEAR,
            COUNT
        };

        using Planes = FlagMap<Side, Plane>;

        // Note(kevinyu):
        // normal of the planes pointing outwards
        Planes planes;

        // if mat is projection matrix only then the resulting frustum will be in view space
        // if mat is view * projection matrix then the resulting frustum will be in world space
        explicit Frustum(const Mat4f& mat);

    	/**
	    * Creates a frustum from 8 corner coordinates.
	    * @param corners the corners of the frustum
	    *
	    * The corners should be specified in this order:
	    * 0. far bottom left
	    * 1. far bottom right
	    * 2. far top left
	    * 3. far top right
	    * 4. near bottom left
	    * 5. near bottom right
	    * 6. near top left
	    * 7. near top right
	    *
	    *     2----3
	    *    /|   /|
	    *   6----7 |
	    *   | 0--|-1      far
	    *   |/   |/       /
	    *   4----5      near
	    *
	    */
        explicit Frustum(const Vec3f corners[8]);
    };

    bool FrustumCull(const Frustum& frustum, const Vec3f& center, const Vec3f& halfExtent);
    bool FrustumCull(const Frustum& frustum, const Vec4f& sphere);

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
    IntersectPointResult IntersectSegmentQuad(Vec3f s1, Vec3f s2, Vec3f q1, Vec3f q2, Vec3f q3, Vec3f q4);
    IntersectPointResult IntersectSegmentTriangle(Vec3f s1, Vec3f s2, Vec3f t1, Vec3f t2, Vec3f t3);

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

    struct BoundingSphere
    {
        Vec3f position;
        float radius = 0.0f;

        BoundingSphere() = default;
    };
    

}