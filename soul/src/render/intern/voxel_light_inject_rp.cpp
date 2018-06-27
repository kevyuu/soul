#include "core/debug.h"

#include "render/type.h"
#include "render/intern/asset.h"
#include "render/intern/util.h"

namespace Soul {
	void VoxelLightInjectRP::init(RenderDatabase& database) {
		program = RenderUtil::GLProgramCreate(RenderAsset::ShaderFile::voxel_light_inject);

		glUseProgram(program);

		GLuint lightDataBlockIndex = glGetUniformBlockIndex(program, "LightData");
		glUniformBlockBinding(program, lightDataBlockIndex, RenderConstant::LIGHT_DATA_BINDING_POINT);

		voxelFrustumCenterLoc = glGetUniformLocation(program, "voxelFrustumCenter");
		voxelFrustumHalfSpanLoc = glGetUniformLocation(program, "voxelFrustumHalfSpan");
		voxelFrustumResoLoc = glGetUniformLocation(program, "voxelFrustumReso");
		voxelAlbedoBufferLoc = glGetUniformLocation(program, "voxelAlbedoBuffer");
		voxelNormalBufferLoc = glGetUniformLocation(program, "voxelNormalBuffer");
		lightVoxelBufferLoc = glGetUniformLocation(program, "lightVoxelBuffer");

		glUseProgram(0);
	}

	void VoxelLightInjectRP::execute(RenderDatabase& db) {



		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(program);

		glUniform1i(voxelFrustumResoLoc, db.voxelFrustumReso);
		glUniform3fv(voxelFrustumCenterLoc, 1, (GLfloat*)&db.voxelFrustumCenter);
		glUniform1f(voxelFrustumHalfSpanLoc, db.voxelFrustumHalfSpan);

		glUniform1i(voxelAlbedoBufferLoc, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, db.voxelGIBuffer.gVoxelAlbedoTex);

		glUniform1i(voxelNormalBufferLoc, 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_3D, db.voxelGIBuffer.gVoxelNormalTex);

		glUniform1i(lightVoxelBufferLoc, 1);
		glBindImageTexture(1, db.voxelGIBuffer.lightVoxelTex, 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);

		glDispatchCompute(db.voxelFrustumReso, db.voxelFrustumReso, db.voxelFrustumReso);

		SOUL_ASSERT(0, RenderUtil::GLIsErrorCheckPass(), "");

	}

	void VoxelLightInjectRP::shutdown(RenderDatabase& database) {

		glDeleteProgram(program);

	}
}