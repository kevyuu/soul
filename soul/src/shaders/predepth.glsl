#ifdef LIB_SHADER
camera.lib.glsl
#endif

/**********************************************************************
// VERTEX_SHADER
**********************************************************************/
#ifdef VERTEX_SHADER

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aBinormal;
layout (location = 4) in vec3 aTangent;

uniform mat4 model;

void main() {
	gl_Position = camera_getProjectionViewMat() * model * vec4(aPos, 1.0f);
}
#endif //VERTEX SHADER

/**********************************************************************
// FRAGMENT_SHADER
**********************************************************************/
#ifdef FRAGMENT_SHADER

void main() {

}
#endif // FRAGMENT_SHADER