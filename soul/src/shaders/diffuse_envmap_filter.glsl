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
    gl_Position = projection * view * vec4(aPos, 1.0f);
    cubeCoords = aPos;
}
#endif // VERTEX_SHADER


/**********************************************************************
// FRAGMENT_SHADER
**********************************************************************/
#ifdef FRAGMENT_SHADER

uniform samplerCube skybox;
in vec3 cubeCoords;
out vec4 fragColor;

void main() {

    float step = M_PI / 8;
    vec3 irradiance = vec3(0, 0, 0);
    float nSample = 0;

    vec3 N = normalize(cubeCoords);
    vec3 up = vec3(0.0f, 1.0f, 0.0f);
    vec3 right = cross(up, N);
    up = cross(N, right);

    for (float phi = 0; phi < 2 * M_PI ; phi += step) {
        for (float theta = 0; theta < M_PI / 2; theta += step) {
            vec3 sampleLocalDir = vec3(cos(theta) * sin (phi), cos(theta) * cos(phi), sin(theta));
            vec3 sampleWorldDir = right * sampleLocalDir.x + up * sampleLocalDir.y + N * sampleLocalDir.z;
            irradiance += texture(skybox, normalize(sampleWorldDir)).rgb * cos(theta) * sin(theta);
            nSample += 1;
        }
    }

    fragColor = vec4(M_PI * (irradiance / nSample), 1.0f);

}
#endif // FRAGMENT_SHADER

