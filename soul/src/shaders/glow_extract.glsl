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

uniform sampler2D lightBuffer;
uniform float threshold;

in VS_OUT {
	vec2 texCoord;
} vs_out;

out vec3 fragColor;

void main() {
	vec3 color = texture(lightBuffer, vs_out.texCoord).rgb;
	if (max(color.r, max(color.g, color.b)) > threshold)
	{
		fragColor = color;	
	}
	else
	{
		fragColor = vec3(0.0f);
	}
}
#endif // FRAGMENT_SHADER