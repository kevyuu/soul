#ifdef LIB_SHADER
math.lib.glsl
brdf.lib.glsl
sampling.lib.glsl
#endif

/**********************************************************************
// VERTEX_SHADER
**********************************************************************/
#ifdef VERTEX_SHADER

layout (location = 0) in vec2 aPos;

out vec2 uv;

void main() {
	gl_Position = vec4(aPos, 0.0f, 1.0f);
	uv = (aPos - vec2(-1.0f, -1.0f)) / 2.0f;
}
#endif // VERTEX_SHADER


/**********************************************************************
// FRAGMENT_SHADER
**********************************************************************/
#ifdef FRAGMENT_SHADER

out vec2 fragColor;

in vec2 uv;

vec2 IntegrateBRDF(float NdotV, float roughness)
{
    vec3 V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.y = 0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    vec3 N = vec3(0.0, 0.0, 1.0f);

    const uint SAMPLE_COUNT = 1024u;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0f);
        float NdotH = max(H.z, 0.0f);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0)
        {
            float G = GeometrySmithIBL(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0f - VdotH, 5.0);

            A += (1.0f - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}

void main() {
    fragColor = IntegrateBRDF(uv.s, uv.t);
}
#endif // FRAGMENT_SHADER
