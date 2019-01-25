#include "core/debug.h"

#include "render/type.h"
#include "render/intern/asset.h"
#include "render/intern/glext.h"

namespace Soul {
	void VoxelLightInjectRP::init(RenderDatabase& database) {
		program = GLExt::ProgramCreate(RenderAsset::ShaderFile::voxel_light_inject);

		glUseProgram(program);

		GLuint lightDataBlockIndex = glGetUniformBlockIndex(program, RenderConstant::LIGHT_DATA_NAME);
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

		SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(program);

		int voxelFrustumReso = db.voxelGIConfig.resolution;

		glUniform1i(voxelFrustumResoLoc, voxelFrustumReso);
		glUniform3fv(voxelFrustumCenterLoc, 1, (GLfloat*)&db.voxelGIConfig.center);
		glUniform1f(voxelFrustumHalfSpanLoc, db.voxelGIConfig.halfSpan);

		glUniform1i(voxelAlbedoBufferLoc, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, db.voxelGIBuffer.gVoxelAlbedoTex);

		glUniform1i(voxelNormalBufferLoc, 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_3D, db.voxelGIBuffer.gVoxelNormalTex);

		glUniform1i(lightVoxelBufferLoc, 1);
		glBindImageTexture(1, db.voxelGIBuffer.lightVoxelTex, 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);

		SOUL_PROFILE_RANGE_PUSH("dispatchCompute()");
		SOUL_LOG(SOUL_LOG_VERBOSE_INFO, "voxelFrustumReso = %d", voxelFrustumReso);
		glDispatchCompute(voxelFrustumReso / 8, voxelFrustumReso / 8, voxelFrustumReso / 8);
		SOUL_PROFILE_RANGE_POP();


		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

		SOUL_PROFILE_RANGE_POP();
	}

	void VoxelLightInjectRP::shutdown(RenderDatabase& database) {

		glDeleteProgram(program);

	}
}