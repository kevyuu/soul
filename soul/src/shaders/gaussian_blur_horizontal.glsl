/**********************************************************************
// VERTEX_SHADER
**********************************************************************/
#ifdef VERTEX_SHADER

layout (location = 0) in vec2 aPos;

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

uniform sampler2D sourceTex;
uniform vec2 targetSizePx;
uniform float lod;

out vec4 fragColor;

in VS_OUT {
	vec2 texCoord;
} vs_out;

void main() {

    vec2 pix_size = 0.5f * 1 / targetSizePx ;
    vec4 color =textureLod( sourceTex,  vs_out.texCoord+vec2( 0.0, 0.0)*pix_size,lod )*0.214607;
	color+=textureLod( sourceTex,  vs_out.texCoord+vec2( 1.0, 0.0)*pix_size,lod )*0.189879;
	color+=textureLod( sourceTex,  vs_out.texCoord+vec2( 2.0, 0.0)*pix_size,lod )*0.157305;
	color+=textureLod( sourceTex,  vs_out.texCoord+vec2( 3.0, 0.0)*pix_size,lod )*0.071303;
	color+=textureLod( sourceTex,  vs_out.texCoord+vec2(-1.0, 0.0)*pix_size,lod )*0.189879;
	color+=textureLod( sourceTex,  vs_out.texCoord+vec2(-2.0, 0.0)*pix_size,lod )*0.157305;
	color+=textureLod( sourceTex,  vs_out.texCoord+vec2(-3.0, 0.0)*pix_size,lod )*0.071303;

    fragColor = color;

}
#endif // FRAGMENT_SHADER