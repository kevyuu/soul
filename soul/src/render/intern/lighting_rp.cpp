//
// Created by Kevin Yudi Utama on 29/8/18.
//

#include "render/data.h"
#include "render/intern/asset.h"
#include "render/intern/glext.h"

namespace Soul {
	namespace Render {

		void LightingRP::init(Database &database) {

			program = GLExt::ProgramCreate(RenderAsset::ShaderFile::lighting);
			GLuint sceneDataBlockIndex = glGetUniformBlockIndex(program, Constant::CAMERA_DATA_NAME);
			glUniformBlockBinding(program, sceneDataBlockIndex, Constant::CAMERA_DATA_BINDING_POINT);
			GLuint lightDataBlockIndex = glGetUniformBlockIndex(program, Constant::LIGHT_DATA_NAME);
			glUniformBlockBinding(program, lightDataBlockIndex, Constant::LIGHT_DATA_BINDING_POINT);

			glUseProgram(program);

			shadowMapUniformLoc = glGetUniformLocation(program, "shadowMap");
			renderMap1UniformLoc = glGetUniformLocation(program, "renderMap1");
			renderMap2UniformLoc = glGetUniformLocation(program, "renderMap2");
			renderMap3UniformLoc = glGetUniformLocation(program, "renderMap3");

			viewPositionUniformLoc = glGetUniformLocation(program, "viewPosition");

			glUseProgram(0);

		}

		void LightingRP::execute(Database& database) {

			glBindFramebuffer(GL_FRAMEBUFFER, database.lightBuffer.frameBuffer);
			glUseProgram(program);

			glUniform1i(shadowMapUniformLoc, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, database.shadowAtlas.texHandle);

			glUniform1i(renderMap1UniformLoc, 1);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, database.gBuffer.renderBuffer1);

			glUniform1i(renderMap2UniformLoc, 2);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, database.gBuffer.renderBuffer2);

			glUniform1i(renderMap3UniformLoc, 3);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, database.gBuffer.renderBuffer3);

			Camera& camera = database.camera;
			glUniform3f(viewPositionUniformLoc, camera.position.x, camera.position.y, camera.position.z);

			glViewport(0, 0, database.targetWidthPx, database.targetHeightPx);
			glBindVertexArray(database.quadVAO);
			glClear(GL_COLOR_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glUseProgram(0);

			GLExt::ErrorCheck("LightingRP::execute");

		}

		void LightingRP::shutdown(Database &database) {

			glDeleteProgram(program);

		}
	}
}