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
uniform sampler2D glowBuffer;
uniform float glowIntensity;
uniform uint glowMask;

in VS_OUT {
	vec2 texCoord;
} vs_out;

out vec3 fragColor;

vec3 Reindhardt(vec3 x)
{
	return x / (max(x.r, max(x.g, x.b)) + 1.0f);
}

vec3 Filmic(vec3 color) {
	// exposure bias: input scale (color *= bias, white *= bias) to make the brightness consistent with other tonemappers
	// also useful to scale the input to the range that the tonemapper is designed for (some require very high input values)
	// has no effect on the curve's general shape or visual properties
	const float exposure_bias = 2.0f;
	const float A = 0.22f * exposure_bias * exposure_bias; // bias baked into constants for performance
	const float B = 0.30f * exposure_bias;
	const float C = 0.10f;
	const float D = 0.20f;
	const float E = 0.01f;
	const float F = 0.30f;

	vec3 color_tonemapped = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white_tonemapped = (((A + C * B) + D * E) / ((A + B) + D * F)) - E / F;

	return clamp(color_tonemapped / white_tonemapped, vec3(0.0f), vec3(1.0f));
}

bool bitTest(uint flags, uint mask) {
	return ((flags & mask) == mask);
}

void main() {
	
	vec3 color = texture(lightBuffer, vs_out.texCoord).rgb;
	vec3 glow = vec3(0.0f, 0.0f, 0.0f);

	if (bitTest(glowMask, 1u << 0))
	{
		glow += texture(glowBuffer, vs_out.texCoord, 1).rgb;
	}
	if (bitTest(glowMask, 1u << 1))
	{
		glow += texture(glowBuffer, vs_out.texCoord, 2).rgb;
	}
	if (bitTest(glowMask, 1u << 2))
	{
		glow += texture(glowBuffer, vs_out.texCoord, 3).rgb;
	}
	if (bitTest(glowMask, 1u << 3))
	{
		glow += texture(glowBuffer, vs_out.texCoord, 4).rgb;
	}
	if (bitTest(glowMask, 1u << 4))
	{
		glow += texture(glowBuffer, vs_out.texCoord, 5).rgb;
	}
	if (bitTest(glowMask, 1u << 5))
	{
		glow += texture(glowBuffer, vs_out.texCoord, 6).rgb;
	}
	if (bitTest(glowMask, 1u << 6))
	{
		glow += texture(glowBuffer, vs_out.texCoord, 7).rgb;
	}
	if (bitTest(glowMask, 1u << 7))
	{
		glow += texture(glowBuffer, vs_out.texCoord, 8).rgb;
	}

	color += (glow * glowIntensity);
	
	color = Filmic(color);
	color = pow(color, vec3(1.0f / 2.2f));
	
	fragColor = color;

}
#endif // FRAGMENT_SHADER