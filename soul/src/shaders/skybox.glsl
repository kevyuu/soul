#define M_PI 3.1415926535897932384626433832795

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
    vec4 position = projection * rot * vec4(aPos, 1.0f);
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

out vec4 fragColor;

void main() {
    fragColor = texture(skybox, normalize(cubeCoords));
}
#endif //FRAGMENT_SHADER