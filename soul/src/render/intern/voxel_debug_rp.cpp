#pragma once

#include "render/intern/glext.h"
#include "render/intern/asset.h"

namespace Soul {
	namespace Render {
		void VoxelDebugRP::init(Database& database) {

			program = GLExt::ProgramCreate(RenderAsset::ShaderFile::voxel_debug);

			GLuint sceneDataBlockIndex = glGetUniformBlockIndex(program, Constant::CAMERA_DATA_NAME);
			glUniformBlockBinding(program, sceneDataBlockIndex, Constant::CAMERA_DATA_BINDING_POINT);

			GLExt::UBOBind(program, Constant::VOXEL_GI_DATA_NAME, Constant::VOXEL_GI_DATA_BINDING_POINT);

			voxelBufferLoc = glGetUniformLocation(program, "voxelBuffer");

			SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

			glGenVertexArrays(1, &dummyVAO);
		}

		void VoxelDebugRP::execute(Database& db) {

			SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glUseProgram(program);

			float fracSize = 1.0f / 4.0f;
			glViewport(0, 0, db.targetWidthPx * fracSize, db.targetHeightPx * fracSize);

			int mipLevel = 0;
			int frac = (int)pow(2, mipLevel);
			glBindImageTexture(0, db.voxelGIBuffer.lightVoxelTex, mipLevel, true, 0, GL_READ_ONLY, GL_RGBA16F);

			glBindVertexArray(dummyVAO);
			glClear(GL_DEPTH_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);

			int voxelReso = db.voxelGIConfig.resolution / frac;
			glDrawArrays(GL_POINTS, dummyVAO, voxelReso * voxelReso * voxelReso);
			SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

			glUseProgram(0);
			glBindVertexArray(0);

			SOUL_PROFILE_RANGE_POP();
		}

		void VoxelDebugRP::shutdown(Database& database) {
			glDeleteProgram(program);
		}
	}
}