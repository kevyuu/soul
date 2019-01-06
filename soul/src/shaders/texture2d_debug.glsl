#ifdef LIB_SHADER
math.lib.glsl
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
out vec4 FragColor;

in VS_OUT {
	vec2 texCoord;
} vs_out;

uniform sampler2D texDebug;

void main() 
{
	vec2 velocity = texture(texDebug, vs_out.texCoord).rg * 2.0f - vec2(1.0f);

	FragColor = vec4(velocity * 1.0f, 0.0f, 1.0f);
}
#endif // FRAGMENT_SHADER