#include "render/type.h"
#include "glext.h"
#include "asset.h"
#include "core/math.h"

namespace Soul {
	void GaussianBlurRP::init(RenderDatabase &database) {

		shaderHorizontal = GLExt::ProgramCreate(RenderAsset::ShaderFile::gaussianBlurHorizontal);
		glUseProgram(shaderHorizontal);
		sourceTexUniformLocHorizontal = glGetUniformLocation(shaderHorizontal, "sourceTex");
		targetSizePxUniformLocHorizontal = glGetUniformLocation(shaderHorizontal, "targetSizePx");
		lodUniformLocHorizontal = glGetUniformLocation(shaderHorizontal, "lod");

		shaderVertical = GLExt::ProgramCreate(RenderAsset::ShaderFile::gaussianBlurVertical);
		glUseProgram(shaderVertical);
		sourceTexUniformLocVertical = glGetUniformLocation(shaderVertical, "sourceTex");
		targetSizePxUniformLocVertical = glGetUniformLocation(shaderVertical, "targetSizePx");
		lodUniformLocVertical = glGetUniformLocation(shaderVertical, "lod");

		glUseProgram(0);
	}

	void GaussianBlurRP::execute(RenderDatabase& database) {
		SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

		RenderDatabase& db = database;

		glBindFramebuffer(GL_READ_FRAMEBUFFER, database.gBuffer.frameBuffer);
		glReadBuffer(GL_COLOR_ATTACHMENT3);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, database.effectBuffer.lightMipChain[0].mipmaps.get(0).frameBuffer);
		glBlitFramebuffer(0, 0,
						  database.targetWidthPx, database.targetHeightPx,
						  0, 0,
						  database.targetWidthPx, database.targetHeightPx,
						  GL_COLOR_BUFFER_BIT, GL_NEAREST);

		for (int i = 0; i < db.effectBuffer.lightMipChain[1].numLevel; i++) {

			Vec2f targetSize = {
					(float) db.effectBuffer.lightMipChain[1].mipmaps.get(i).width,
					(float) db.effectBuffer.lightMipChain[1].mipmaps.get(i).height
			};
			glViewport(0, 0,
					   (int)targetSize.x,
					   (int)targetSize.y);

			// Horizontal Blur Pass
			glUseProgram(shaderHorizontal);
			glBindFramebuffer(GL_FRAMEBUFFER, db.effectBuffer.lightMipChain[1].mipmaps.get(i).frameBuffer);
			glUniform1i(sourceTexUniformLocHorizontal, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, db.effectBuffer.lightMipChain[0].colorBuffer);
			glUniform2f(targetSizePxUniformLocHorizontal,
					targetSize.x,
					targetSize.y);
			glUniform1f(lodUniformLocHorizontal, float(i));
			glBindVertexArray(database.quadVAO);
			glClear(GL_COLOR_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			// Vertical Blur Pass
			glUseProgram(shaderVertical);
			glBindFramebuffer(GL_FRAMEBUFFER, db.effectBuffer.lightMipChain[0].mipmaps.get(i + 1).frameBuffer);
			glUniform1i(sourceTexUniformLocVertical, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, db.effectBuffer.lightMipChain[1].colorBuffer);
			glUniform2f(targetSizePxUniformLocVertical,
						targetSize.x,
						targetSize.y);
			glUniform1f(lodUniformLocVertical, float(i));
			glBindVertexArray(database.quadVAO);
			glClear(GL_COLOR_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		}

		glUseProgram(0);

		GLExt::ErrorCheck("GaussianBlurRP::execute");

		SOUL_PROFILE_RANGE_POP();
	}

	void GaussianBlurRP::shutdown(Soul::RenderDatabase &database) {
		glDeleteProgram(shaderHorizontal);
		glDeleteProgram(shaderVertical);
	}
}