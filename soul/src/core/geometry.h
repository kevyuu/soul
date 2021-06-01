#include "core/type.h"

namespace Soul {
    struct TangentFrameComputeInput {
        uint64 vertexCount = 0;
        Vec3f* normals = nullptr;
        Vec4f* tangents = nullptr;
        Vec2f* uvs = nullptr;
        Vec3f* positions = nullptr;
        Vec3ui32* triangles32 = nullptr;
        uint64 triangleCount = 0;
    };

    bool ComputeTangentFrame(const TangentFrameComputeInput& input, Quaternion* quat);
}