//
// Created by Kevin Yudi Utama on 29/8/18.
//

#include "render/data.h"
#include "render/intern/asset.h"
#include "render/intern/glext.h"
#include "core/math.h"

namespace Soul {
	namespace Render {

		void GlowExtractRP::init(Database& database) {

			program = GLExt::ProgramCreate(RenderAsset::ShaderFile::glow_extract);

			lightBufferLoc = glGetUniformLocation(program, "lightBuffer");
			thresholdLoc = glGetUniformLocation(program, "threshold");

		}

		void GlowExtractRP::execute(Database &db) {
			SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

			Camera& camera = db.camera;

			glBindFramebuffer(GL_FRAMEBUFFER, db.effectBuffer.lightMipChain[0].mipmaps[0].frameBuffer);
			glUseProgram(program);

			GLExt::ErrorCheck("GlowExtractRP::execute");

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, db.lightBuffer.colorBuffer);

			GLExt::ErrorCheck("GlowExtractRP::execute");

			glUniform1f(thresholdLoc, db.postProcessConfig.glowConfig.threshold);

			glViewport(0, 0, db.targetWidthPx, db.targetHeightPx);
			glBindVertexArray(db.quadVAO);
			glClear(GL_COLOR_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			GLExt::ErrorCheck("GlowExtractRP::execute");
			
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glUseProgram(0);

			GLExt::ErrorCheck("GlowExtractRP::execute");

			SOUL_PROFILE_RANGE_POP();
		}

		void GlowExtractRP::shutdown(Database &database) {
			GLExt::ProgramDelete(&program);
		}

	}
}