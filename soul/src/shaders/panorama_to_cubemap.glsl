#ifdef LIB_SHADER
math.lib.glsl
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
uniform sampler2D panorama;

out vec4 fragColor;

vec4 texturePanorama(sampler2D panorama, vec3 texCoord) {
    float phi = atan(texCoord.z, texCoord.x);
    float thetha = asin(texCoord.y);
    float u = (phi / (2 * M_PI)) + 0.5f;
    float v = (thetha / M_PI) + 0.5f;

    vec3 envColor = texture(panorama, vec2(u, v)).rgb;
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0 / 2.2));

    return vec4(envColor, 1.0f);
}

void main() {
    fragColor = texturePanorama(panorama, normalize(cubeCoords));
}
#endif // FRAGMENT_SHADER

