#version 450

struct PostProcessInputs
{
    float depth;
};

layout(set = 0, binding = 0) uniform sampler2D materialParams_depth;

void postProcess(inout PostProcessInputs postProcess_1)
{
    int level = 0;
    ivec2 icoord = ivec2(gl_FragCoord.xy);
    postProcess_1.depth = texelFetch(materialParams_depth, (ivec2(2) * icoord) + ivec2(icoord.y & 1, icoord.x & 1), level).x;
}

void main()
{
    PostProcessInputs inputs;
    PostProcessInputs param = inputs;
    postProcess(param);
    inputs = param;
    gl_FragDepth = inputs.depth;
}
