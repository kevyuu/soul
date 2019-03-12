#include "render/data.h"
#include "render/intern/glext.h"
#include "render/intern/asset.h"
#include "core/math.h"

namespace Soul {
	namespace Render {
		void SkyboxRP::init(Database& database) {

			shader = GLExt::ProgramCreate(RenderAsset::ShaderFile::skybox);

			projectionLoc = glGetUniformLocation(shader, "projection");
			viewLoc = glGetUniformLocation(shader, "view");
			skyboxLoc = glGetUniformLocation(shader, "skybox");

		}

		void SkyboxRP::execute(Database& database) {

			SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glUseProgram(shader);

			glUniform1i(skyboxLoc, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, database.environment.cubemap);

			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, (const GLfloat*)database.cameraDataUBO.view.elem);
			glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, (const GLfloat*)database.cameraDataUBO.projection.elem);

			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);

			glBindVertexArray(database.cubeVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glBindVertexArray(0);

			glUseProgram(0);

			SOUL_PROFILE_RANGE_POP();
		}

		void SkyboxRP::shutdown(Database& database) {
			glDeleteProgram(shader);
		}
	}
}