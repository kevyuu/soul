#include "draw_item.h"

namespace soul_fila
{
	void DrawItem::ToPipelineStateDesc(const DrawItem& drawItem, soul::gpu::GraphicPipelineStateDesc& desc)
	{
        desc.programID = drawItem.programID;
		const RasterState& rasterState = drawItem.rasterState;
		desc.raster.cullMode = rasterState.culling;
		desc.raster.frontFace = rasterState.inverseFrontFaces ? soul::gpu::FrontFace::CLOCKWISE : soul::gpu::FrontFace::COUNTER_CLOCKWISE;

		if (desc.colorAttachmentCount > 0)
		{
			SOUL_ASSERT(0, desc.colorAttachmentCount == 1, "");
			auto& colorAttch = desc.colorAttachments[0];
            colorAttch.blendEnable = rasterState.hasBlending();
			colorAttch.colorWrite = rasterState.colorWrite;
			colorAttch.colorBlendOp = rasterState.blendEquationRGB;
			colorAttch.alphaBlendOp = rasterState.blendEquationAlpha;
			colorAttch.srcColorBlendFactor = rasterState.blendFunctionSrcRGB;
			colorAttch.dstColorBlendFactor = rasterState.blendFunctionDstRGB;
			colorAttch.srcAlphaBlendFactor = rasterState.blendFunctionSrcAlpha;
			colorAttch.dstAlphaBlendFactor = rasterState.blendFunctionDstAlpha;
		}

		desc.depthStencilAttachment.depthWriteEnable = rasterState.depthWrite;
		desc.depthStencilAttachment.depthCompareOp = rasterState.depthFunc;
		
        gpu::GraphicPipelineStateDesc::InputBindingDesc inputBindings[gpu::MAX_INPUT_BINDING_PER_SHADER];
        gpu::GraphicPipelineStateDesc::InputAttrDesc inputAttributes[gpu::MAX_INPUT_PER_SHADER];

        const Primitive& primitive = *drawItem.primitive;
        for (uint32 attribIdx = 0; attribIdx < to_underlying(VertexAttribute::COUNT); attribIdx++) {
            Attribute attribute = primitive.attributes[attribIdx];
            gpu::VertexElementFlags elemFlags = attribute.elementFlags;
            gpu::VertexElementType elemType = attribute.elementType;
            if (attribute.buffer == Attribute::BUFFER_UNUSED) {
                attribute = primitive.attributes[0];
                if (attribIdx == to_underlying(VertexAttribute::BONE_INDICES)) {
                    elemFlags = gpu::VERTEX_ELEMENT_INTEGER_TARGET;
                    elemType = gpu::VertexElementType::UBYTE4;
                }
                else {
                    elemFlags = gpu::VERTEX_ELEMENT_NORMALIZED;
                    elemType = gpu::VertexElementType::BYTE4;
                }
            }

            inputBindings[attribIdx] = { attribute.stride };
            inputAttributes[attribIdx] = {
                attribIdx,
                attribute.offset,
                elemType,
                elemFlags
            };
        }
        memcpy(desc.inputBindings, inputBindings, sizeof(inputBindings));
        memcpy(desc.inputAttributes, inputAttributes, sizeof(inputAttributes));
	}
}