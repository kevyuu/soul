#include "render/data.h"
#include "render/intern/asset.h"
#include "render/intern/glext.h"

namespace Soul {
	namespace Render {

		void Texture2DDebugRP::init(Database& database) {

			program = GLExt::ProgramCreate(RenderAsset::ShaderFile::texture2dDebug);

		}

		void Texture2DDebugRP::execute(Database& database) {

			SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

			Camera& camera = database.camera;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			float fracSize = 1.0f / 4;
			glViewport(0, 0, database.targetWidthPx * fracSize, database.targetHeightPx * fracSize);
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glUseProgram(program);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, database.velocityBuffer.tex);
			glBindVertexArray(database.quadVAO);
			glDisable(GL_DEPTH_TEST);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindVertexArray(0);
			glUseProgram(0);

			SOUL_PROFILE_RANGE_POP();
		}

		void Texture2DDebugRP::shutdown(Database &database) {
			glDeleteProgram(program);
		}


	}
}