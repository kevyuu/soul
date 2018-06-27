#define M_PI 3.1415926535897932384626433832795

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
out vec4 FragColor;

in VS_OUT {
	vec2 texCoord;
} vs_out;

uniform sampler2D texDebug;

void main() 
{
	FragColor = vec4(texture(texDebug, vs_out.texCoord).rgb, 1.0f);
}
#endif // FRAGMENT_SHADER