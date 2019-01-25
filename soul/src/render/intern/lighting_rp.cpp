//
// Created by Kevin Yudi Utama on 29/8/18.
//

#include "render/type.h"
#include "render/intern/asset.h"
#include "render/intern/glext.h"

namespace Soul {

	void LightingRP::init(RenderDatabase &database) {

		shader = GLExt::ProgramCreate(RenderAsset::ShaderFile::lighting);
		GLuint sceneDataBlockIndex = glGetUniformBlockIndex(shader, RenderConstant::CAMERA_DATA_NAME);
		glUniformBlockBinding(shader, sceneDataBlockIndex, RenderConstant::CAMERA_DATA_BINDING_POINT);
		GLuint lightDataBlockIndex = glGetUniformBlockIndex(shader, RenderConstant::LIGHT_DATA_NAME);
		glUniformBlockBinding(shader, lightDataBlockIndex, RenderConstant::LIGHT_DATA_BINDING_POINT);

		glUseProgram(shader);

		shadowMapUniformLoc = glGetUniformLocation(shader, "shadowMap");
		renderMap1UniformLoc = glGetUniformLocation(shader, "renderMap1");
		renderMap2UniformLoc = glGetUniformLocation(shader, "renderMap2");
		renderMap3UniformLoc = glGetUniformLocation(shader, "renderMap3");

		viewPositionUniformLoc = glGetUniformLocation(shader, "viewPosition");

		glUseProgram(0);

	}

	void LightingRP::execute(RenderDatabase& database) {

		glBindFramebuffer(GL_FRAMEBUFFER, database.lightBuffer.frameBuffer);
		glUseProgram(shader);

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

	void LightingRP::shutdown(RenderDatabase &database) {

		glDeleteProgram(shader);

	}


}