#include "render/intern/asset.h"
#include "render/data.h"
#include "render/intern/glext.h"
#include "core/debug.h"
#include "core/math.h"

#include <iostream>

namespace Soul {
	namespace Render {
		void VelocityBufferGenRP::init(Database& database) {
			SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");
			program = GLExt::ProgramCreate(RenderAsset::ShaderFile::velocityBufferGen);

			depthMapLoc = glGetUniformLocation(program, "depthMap");
			invCurProjectionViewLoc = glGetUniformLocation(program, "invCurProjectionView");
			prevProjectionViewLoc = glGetUniformLocation(program, "prevProjectionView");

			SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");
		}

		void VelocityBufferGenRP::execute(Database& db) {
			SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

			glViewport(0, 0, db.targetWidthPx, db.targetHeightPx);
			glBindFramebuffer(GL_FRAMEBUFFER, db.velocityBuffer.frameBuffer);
			glUseProgram(program);

			glUniform1i(depthMapLoc, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, db.gBuffer.depthBuffer);

			Mat4 viewMat = mat4View(db.camera.position, db.camera.position +
				db.camera.direction, db.camera.up);
			Mat4 curProjectionViewMat = db.camera.projection * viewMat;
			Mat4 curInvProjectionViewMat = mat4Inverse(curProjectionViewMat);

			Mat4 prevViewMat = mat4View(db.prevCamera.position, db.prevCamera.position +
				db.prevCamera.direction, db.prevCamera.up);
			Mat4 prevProjectionViewMat = db.prevCamera.projection * prevViewMat;

			glUniformMatrix4fv(invCurProjectionViewLoc, 1, GL_TRUE, (const GLfloat*)curInvProjectionViewMat.elem);
			glUniformMatrix4fv(prevProjectionViewLoc, 1, GL_TRUE, (const GLfloat*)prevProjectionViewMat.elem);

			glBindVertexArray(db.quadVAO);
			glClear(GL_COLOR_BUFFER_BIT);
			glDepthMask(GL_FALSE);
			glDisable(GL_DEPTH_TEST);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glDepthMask(GL_TRUE);
			glEnable(GL_DEPTH_TEST);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glUseProgram(0);

			SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");
			SOUL_PROFILE_RANGE_POP();
		}

		void VelocityBufferGenRP::shutdown(Database& database) {
			GLExt::ProgramDelete(&program);
		}

	}
}