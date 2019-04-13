//
// Created by Kevin Yudi Utama on 29/8/18.
//

#include "render/data.h"
#include "render/intern/asset.h"
#include "render/intern/glext.h"
#include "core/math.h"

namespace Soul {
	namespace Render {

		void GlowBlendRP::init(Database& database) {

			program = GLExt::ProgramCreate(RenderAsset::ShaderFile::glow_blend);

			lightBufferLoc = glGetUniformLocation(program, "lightBuffer");
			glowBufferLoc = glGetUniformLocation(program, "glowBuffer");
			glowIntensityLoc = glGetUniformLocation(program, "glowIntensity");
			glowMaskLoc = glGetUniformLocation(program, "glowMask");
			exposureLoc = glGetUniformLocation(program, "exposure");

		}

		static uint32 _buildGlowMask(GlowConfig& glowConfig)
		{
			uint32 mask = 0;
			for (int i = 0; i < sizeof(glowConfig.useLevel); i++)
			{
				if (glowConfig.useLevel[i])
				{
					mask |= (1u << i);
				}
			}
			return mask;
		}

		void GlowBlendRP::execute(Database &db) {
			SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

			Camera& camera = db.camera;

			glBindFramebuffer(GL_FRAMEBUFFER, db.effectBuffer.postProcessBuffer.frameBuffer);
			glUseProgram(program);

			glUniform1i(lightBufferLoc, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, db.lightBuffer.colorBuffer);

			glUniform1i(glowBufferLoc, 1);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, db.effectBuffer.lightMipChain[0].colorBuffer);

			glUniform1f(glowIntensityLoc, db.postProcessConfig.glowConfig.intensity);
			glUniform1ui(glowMaskLoc, _buildGlowMask(db.postProcessConfig.glowConfig));
			glUniform1f(exposureLoc, db.environment.exposure);

			glViewport(0, 0, db.targetWidthPx, db.targetHeightPx);
			glBindVertexArray(db.quadVAO);
			glClear(GL_COLOR_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, db.effectBuffer.postProcessBuffer.frameBuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(0, 0, db.targetWidthPx, db.targetHeightPx, 0, 0, db.targetWidthPx, db.targetHeightPx, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, db.gBuffer.frameBuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(0, 0, db.targetWidthPx, db.targetHeightPx, 0, 0, db.targetWidthPx, db.targetHeightPx, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glUseProgram(0);

			GLExt::ErrorCheck("GlowBlendRP::execute");

			SOUL_PROFILE_RANGE_POP();
		}

		void GlowBlendRP::shutdown(Database &database) {
			GLExt::ProgramDelete(&program);
		}

	}
}