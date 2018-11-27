#ifdef LIB_SHADER
math.lib.glsl
sampling.lib.glsl
#endif

/**********************************************************************
// VERTEX_SHADER
**********************************************************************/
#ifdef VERTEX_SHADER
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec3 cubeCoords;

void main() {
    mat4 rot = mat4(mat3(view));
    vec4 position = projection * rot * vec4(aPos, 1.0);
    gl_Position = position.xyww;
    cubeCoords = aPos;
}
#endif // VERTEX_SHADER


/**********************************************************************
// FRAGMENT_SHADER
**********************************************************************/
#ifdef FRAGMENT_SHADER

in vec3 cubeCoords;

uniform samplerCube skybox;
uniform float roughness;

out vec4 fragColor;

void main()
{
    vec3 N = normalize(cubeCoords);
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 0.0;
    vec3 prefilteredColor = vec3(0.0);
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            prefilteredColor += texture(skybox, L).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;

    fragColor = vec4(prefilteredColor, 1.0);
}
#endif // FRAGMENT_SHADER

