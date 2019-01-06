/**********************************************************************
// VERTEX_SHADER
**********************************************************************/
#ifdef VERTEX_SHADER

layout(location = 0) in vec2 aPos;

out VS_OUT{
	vec2 texCoord;
} vs_out;

void main() {
	gl_Position = vec4(aPos, 0.0f, 1.0f);
	vs_out.texCoord = (aPos + vec2(1.0f)) / 2.0f;
}
#endif //VERTEX SHADER

/**********************************************************************
// FRAGMENT_SHADER
**********************************************************************/
#ifdef FRAGMENT_SHADER

uniform sampler2D depthMap;
uniform mat4 invCurProjectionView;
uniform mat4 prevProjectionView;

in VS_OUT{
	vec2 texCoord;
} vs_out;

out vec2 velocity;

void main() {
	
	float ndcDepth = texture(depthMap, vs_out.texCoord).r;
	vec3 curNdcPos = vec3(vs_out.texCoord , ndcDepth) * 2 - vec3(1.0f);
	vec4 worldPos = invCurProjectionView * vec4(curNdcPos, 1.0f);
	worldPos /= worldPos.w;
	vec4 prevNdcPos = prevProjectionView * worldPos;
	prevNdcPos /= prevNdcPos.w;

	velocity = vec2(curNdcPos.xy - prevNdcPos.xy) * 0.5f + vec2(0.5f);

}
#endif // FRAGMENT_SHADER