#include "core/dev_util.h"

#include "render/data.h"
#include "render/intern/asset.h"
#include "render/intern/glext.h"

namespace Soul {
	namespace Render {
		void VoxelLightInjectRP::init(Database& database) {
			program = GLExt::ProgramCreate(RenderAsset::ShaderFile::voxel_light_inject);

			glUseProgram(program);

			GLuint lightDataBlockIndex = glGetUniformBlockIndex(program, Constant::LIGHT_DATA_NAME);
			glUniformBlockBinding(program, lightDataBlockIndex, Constant::LIGHT_DATA_BINDING_POINT);

			GLExt::UBOBind(program, Constant::VOXEL_GI_DATA_NAME, Constant::VOXEL_GI_DATA_BINDING_POINT);

			voxelAlbedoBufferLoc = glGetUniformLocation(program, "voxelAlbedoBuffer");
			voxelNormalBufferLoc = glGetUniformLocation(program, "voxelNormalBuffer");
			voxelEmissiveBufferLoc = glGetUniformLocation(program, "voxelEmissiveBuffer");
			lightVoxelBufferLoc = glGetUniformLocation(program, "lightVoxelBuffer");
			emissiveScaleLoc = glGetUniformLocation(program, "emissiveScale");

			glUseProgram(0);
		}

		void VoxelLightInjectRP::execute(Database& db) {

			SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

			float data[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

			glClearTexImage(db.voxelGIBuffer.lightVoxelTex, 0, GL_RGBA, GL_FLOAT, data);
			glClearTexImage(db.voxelGIBuffer.lightVoxelTex, 0, GL_RGBA, GL_FLOAT, data);
			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			glUseProgram(program);

			int voxelFrustumReso = db.voxelGIConfig.resolution;

			glUniform1i(voxelAlbedoBufferLoc, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_3D, db.voxelGIBuffer.gVoxelAlbedoTex);

			glUniform1i(voxelNormalBufferLoc, 1);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_3D, db.voxelGIBuffer.gVoxelNormalTex);

			glUniform1i(voxelEmissiveBufferLoc, 2);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_3D, db.voxelGIBuffer.gVoxelEmissiveTex);

			glUniform1i(lightVoxelBufferLoc, 3);
			glBindImageTexture(3, db.voxelGIBuffer.lightVoxelTex, 0, false, 0, GL_WRITE_ONLY, GL_RGBA16F);

			glUniform1f(emissiveScaleLoc, db.environment.emissiveScale);

			SOUL_PROFILE_RANGE_PUSH("dispatchCompute()");
			glDispatchCompute(voxelFrustumReso / 8, voxelFrustumReso / 8, voxelFrustumReso / 8);
			SOUL_PROFILE_RANGE_POP();

			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

			SOUL_PROFILE_RANGE_POP();
		}

		void VoxelLightInjectRP::shutdown(Database& database) {

			glDeleteProgram(program);

		}
	}
}