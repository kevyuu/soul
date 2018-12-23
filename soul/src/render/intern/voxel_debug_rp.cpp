#pragma once

#include "render/intern/glext.h"
#include "render/intern/asset.h"

namespace Soul {

	void VoxelDebugRP::init(RenderDatabase& database) {
		program = GLExt::ProgramCreate(RenderAsset::ShaderFile::voxel_debug);

		GLuint sceneDataBlockIndex = glGetUniformBlockIndex(program, "SceneData");
		glUniformBlockBinding(program, sceneDataBlockIndex, RenderConstant::SCENE_DATA_BINDING_POINT);

		voxelBufferLoc = glGetUniformLocation(program, "voxelBuffer");
		voxelFrustumResoLoc = glGetUniformLocation(program, "voxelFrustumReso");
		voxelFrustumCenterLoc = glGetUniformLocation(program, "voxelFrustumCenter");
		voxelFrustumHalfSpanLoc = glGetUniformLocation(program, "voxelFrustumHalfSpan");

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

		glGenVertexArrays(1, &dummyVAO);
	}

	void VoxelDebugRP::execute(RenderDatabase& db) {

		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUseProgram(program);

		float fracSize = 1.0f / 4.0f;
		glViewport(0, 0, db.targetWidthPx * fracSize, db.targetHeightPx * fracSize);

		int mipLevel = 0;
		int frac = (int)pow(2, mipLevel);
		glBindImageTexture(0, db.voxelGIBuffer.lightVoxelTex, mipLevel, true, 0, GL_READ_ONLY, GL_RGBA16F);

		glUniform1i(voxelFrustumResoLoc, db.voxelFrustumReso / frac);
		glUniform3fv(voxelFrustumCenterLoc, 1, (GLfloat*)&db.voxelFrustumCenter);
		glUniform1f(voxelFrustumHalfSpanLoc, db.voxelFrustumHalfSpan);

		glBindVertexArray(dummyVAO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);

		int voxelReso = db.voxelFrustumReso / frac;
		glDrawArrays(GL_POINTS, dummyVAO, voxelReso * voxelReso * voxelReso);
		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

		glUseProgram(0);
		glBindVertexArray(0);

	}

	void VoxelDebugRP::shutdown(RenderDatabase& database) {
		glDeleteProgram(program);
	}
}