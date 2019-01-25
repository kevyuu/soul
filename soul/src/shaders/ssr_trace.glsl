#ifdef LIB_SHADER
math.lib.glsl
sampling.lib.glsl
#endif

/**********************************************************************
// VERTEX_SHADER
**********************************************************************/
#ifdef VERTEX_SHADER

layout(location = 0) in vec2 aPos;

out VS_OUT {
    vec2 texCoord;
} vs_out;

void main() {
    gl_Position = vec4(aPos, 0.0f, 1.0f);
    vs_out.texCoord = (aPos - vec2(-1.0f, -1.0f)) / 2.0f;
}
#endif //VERTEX SHADER

/**********************************************************************
// FRAGMENT_SHADER
**********************************************************************/
#ifdef FRAGMENT_SHADER

#define MAX_STEP 32

uniform sampler2D renderMap1;
uniform sampler2D renderMap2;
uniform sampler2D renderMap3;
uniform sampler2D depthMap;

uniform vec2 screenDimension;

uniform vec3 cameraPosition;
uniform float cameraZNear;
uniform float cameraZFar;

uniform mat4 invProjectionView;

layout(std140) uniform SceneData {
	mat4 projection;
	mat4 view;
};

in VS_OUT {
	vec2 texCoord;
} vs_out;

out vec2 reflectionPos;

float ndcDepthToViewZ(float ndcDepth) {
    return -2.0 * cameraZNear * cameraZFar / (cameraZFar + cameraZNear - ndcDepth * (cameraZFar - cameraZNear));
}

void main() {

	float ndcDepth = texture(depthMap, vs_out.texCoord).r * 2.0f - 1.0f;
	vec4 worldPositionHomo = invProjectionView * vec4(vs_out.texCoord * 2.0f - 1.0f, ndcDepth, 1.0f);
	vec3 worldPosition = (worldPositionHomo / worldPositionHomo.w).xyz;
	vec3 N = texture(renderMap3, vs_out.texCoord).rgb * 2.0f - 1.0f;
	float roughness = texture(renderMap3, vs_out.texCoord).a;

	vec3 V = normalize(cameraPosition - worldPosition);

	uint columnIdx = uint(floor(vs_out.texCoord.x / screenDimension.x));
	uint rowIdx = uint(floor(vs_out.texCoord.y / screenDimension.y));

	uint pixelIdx = uint(rowIdx * screenDimension.y + columnIdx);
	uint pixelCount = uint(screenDimension.x * screenDimension.y);

    vec2 rand = Hammersley(pixelIdx, pixelCount);
    rand.x = mix(rand.x, 1.0f, 0.4f);
    vec3 H = ImportanceSampleGGX(rand, N, roughness);
    vec3 viewRayDir = mat3(view) * normalize(2.0 * dot(V, H) * H - V);

    if (viewRayDir.z >= 0) {
        reflectionPos = vec2(0.0f, 0.0f);
        return;
    }

    vec4 viewRayStart = view * vec4(worldPosition, 1.0);
    float viewRayLength = (viewRayStart.z + viewRayDir.z * cameraZFar) > - cameraZNear ? (-cameraZNear - viewRayStart.z) / viewRayDir.z : cameraZFar;
    vec3 viewRayEnd = viewRayStart.xyz + viewRayDir * viewRayLength;

    vec4 ndcRayStart = projection * viewRayStart;
    float invWStart = 1 / ndcRayStart.w;
    ndcRayStart *= invWStart;
    vec2 halfScreenDimension = screenDimension * 0.5f;
    vec2 screenRayStart = ndcRayStart.xy * halfScreenDimension + halfScreenDimension;
    vec4 ndcRayEnd = projection * vec4(viewRayEnd, 1.0f);
    float invWEnd = 1 / ndcRayEnd.w;
    ndcRayEnd *= invWEnd;
    vec2 screenRayEnd = ndcRayEnd.xy * halfScreenDimension + halfScreenDimension;
    vec2 screenRayDelta = screenRayEnd - screenRayStart;
    vec2 screenRayStep = normalize(screenRayDelta);
    screenRayStep = screenRayStep / max(abs(screenRayStep.x), abs(screenRayStep.y));

    screenRayStep *= 64;

    float fracStep = length(screenRayStep) / length(screenRayDelta);
    float invWStep = (invWEnd - invWStart) * fracStep;

    float viewRayZIterStart = viewRayStart.z * invWStart;
    float viewRayZIterEnd = viewRayEnd.z * invWEnd;
    float viewRayZIterStep = (viewRayZIterEnd - viewRayZIterStart) * fracStep;

    vec2 screenRayIter = screenRayStart;
    float viewRayZIter = viewRayZIterStart;
    float invWIter = invWStart;

    vec2 invScreenDimension = 1 / screenDimension;
    reflectionPos = vec2(0.0f, 0.0f);

    int i = 0;
    for (i = 0; i < MAX_STEP; i++) {
        screenRayIter += screenRayStep;
        viewRayZIter += viewRayZIterStep;
        invWIter += invWStep;

        float curViewRayZ = viewRayZIter / invWIter;

        vec2 uv = screenRayIter * invScreenDimension;

        float ndcDepth = texture(depthMap, uv).r * 2.0 - 1.0;

        float sceneViewZ = ndcDepthToViewZ(ndcDepth);

        if (curViewRayZ < sceneViewZ) {
            float binaryMult = 1;
            for (int j = i; j < MAX_STEP; j++) {
                float signStep = sign(curViewRayZ - sceneViewZ);
                binaryMult *= 0.5;
                screenRayIter += screenRayStep * signStep * binaryMult;
                viewRayZIter += viewRayZIterStep * signStep * binaryMult;
                invWIter += invWStep * signStep * binaryMult;

                curViewRayZ = viewRayZIter / invWIter;
                uv = screenRayIter * invScreenDimension;
                ndcDepth = texture(depthMap, uv).r * 2.0f - 1.0f;
                sceneViewZ = ndcDepthToViewZ(ndcDepth);
            }

            reflectionPos = sceneViewZ - curViewRayZ > 2.4f ? vec2(0.0f, 0.0f) : uv;
            break;
        }
    }

}
#endif // FRAGMENT_SHADER