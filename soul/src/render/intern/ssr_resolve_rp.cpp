#include <render/data.h>
#include "render/data.h"
#include "render/intern/asset.h"
#include "render/intern/glext.h"
#include "core/math.h"

namespace Soul {
	namespace Render {

		void SSRResolveRP::init(Database& database) {

			program = GLExt::ProgramCreate(RenderAsset::ShaderFile::ssrResolve);

			GLuint sceneDataBlockIndexPredepth = glGetUniformBlockIndex(program, Constant::CAMERA_DATA_NAME);
			glUniformBlockBinding(program, sceneDataBlockIndexPredepth, Constant::CAMERA_DATA_BINDING_POINT);

			GLExt::UBOBind(program, Constant::VOXEL_GI_DATA_NAME, Constant::VOXEL_GI_DATA_BINDING_POINT);

			reflectionPosBufferLoc = glGetUniformLocation(program, "reflectionPosBuffer");
			lightBufferLoc = glGetUniformLocation(program, "lightBuffer");
			renderMap1Loc = glGetUniformLocation(program, "renderMap1");
			renderMap2Loc = glGetUniformLocation(program, "renderMap2");
			renderMap3Loc = glGetUniformLocation(program, "renderMap3");
			renderMap4Loc = glGetUniformLocation(program, "renderMap4");
			depthMapLoc = glGetUniformLocation(program, "depthMap");
			FGMapLoc = glGetUniformLocation(program, "FGMap");
			voxelLightBufferLoc = glGetUniformLocation(program, "voxelLightBuffer");

			screenDimensionLoc = glGetUniformLocation(program, "screenDimension");

			diffuseEnvTexLoc = glGetUniformLocation(program, "diffuseEnvTex");
			specularEnvTexLoc = glGetUniformLocation(program, "specularEnvTex");

			GLExt::ErrorCheck("SSRResolveRP::init");
		}

		void SSRResolveRP::execute(Database &db) {

			SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			glBindFramebuffer(GL_FRAMEBUFFER, db.lightBuffer.frameBuffer);
			glUseProgram(program);

			glUniform1i(reflectionPosBufferLoc, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, db.effectBuffer.ssrTraceBuffer.traceBuffer);

			glUniform1i(lightBufferLoc, 1);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, db.effectBuffer.lightMipChain[0].colorBuffer);

			glUniform1i(renderMap1Loc, 2);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, db.gBuffer.renderBuffer1);

			glUniform1i(renderMap2Loc, 3);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, db.gBuffer.renderBuffer2);

			glUniform1i(renderMap3Loc, 4);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, db.gBuffer.renderBuffer3);

			glUniform1i(renderMap4Loc, 5);
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, db.gBuffer.renderBuffer4);

			glUniform1i(depthMapLoc, 6);
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_2D, db.gBuffer.depthBuffer);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

			glUniform1i(FGMapLoc, 7);
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_2D, db.environment.brdfMap);

			glUniform1i(voxelLightBufferLoc, 8);
			glActiveTexture(GL_TEXTURE8);
			glBindTexture(GL_TEXTURE_3D, db.voxelGIBuffer.lightVoxelTex);

			glUniform1i(diffuseEnvTexLoc, 9);
			glActiveTexture(GL_TEXTURE9);
			glBindTexture(GL_TEXTURE_CUBE_MAP, db.environment.diffuseMap);

			glUniform1i(specularEnvTexLoc, 10);
			glActiveTexture(GL_TEXTURE10);
			glBindTexture(GL_TEXTURE_CUBE_MAP, db.environment.specularMap);

			glUniform2f(screenDimensionLoc, db.targetWidthPx, db.targetHeightPx);

			glViewport(0, 0, db.targetWidthPx, db.targetHeightPx);
			glBindVertexArray(db.quadVAO);
			glClear(GL_COLOR_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, db.lightBuffer.frameBuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(0, 0, db.targetWidthPx, db.targetHeightPx, 0, 0, db.targetWidthPx, db.targetHeightPx, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, db.gBuffer.frameBuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(0, 0, db.targetWidthPx, db.targetHeightPx, 0, 0, db.targetWidthPx, db.targetHeightPx, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glUseProgram(0);

			GLExt::ErrorCheck("SSRResolveRP::execute");

			SOUL_PROFILE_RANGE_POP();
		}

		void SSRResolveRP::shutdown(Database &database) {
			glDeleteProgram(program);
		}
	}
}