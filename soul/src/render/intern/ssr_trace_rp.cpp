#include <render/type.h>
#include "render/type.h"
#include "render/intern/asset.h"
#include "render/intern/glext.h"
#include "core/math.h"

namespace Soul {

	void SSRTraceRP::init(RenderDatabase& database) {
		shader = GLExt::ProgramCreate(RenderAsset::ShaderFile::ssrTrace);

		GLuint sceneDataBlockIndex = glGetUniformBlockIndex(shader, "SceneData");
		glUniformBlockBinding(shader, sceneDataBlockIndex, RenderConstant::SCENE_DATA_BINDING_POINT);

		renderMap1UniformLoc = glGetUniformLocation(shader, "renderMap1");
		renderMap2UniformLoc = glGetUniformLocation(shader, "renderMap2");
		renderMap3UniformLoc = glGetUniformLocation(shader, "renderMap3");
		depthMapLoc = glGetUniformLocation(shader, "depthMap");
		screenDimensionLoc = glGetUniformLocation(shader, "screenDimension");
		cameraPositionLoc = glGetUniformLocation(shader, "cameraPosition");
		cameraZNearLoc = glGetUniformLocation(shader, "cameraZNear");
		cameraZFarLoc = glGetUniformLocation(shader, "cameraZFar");

		invProjectionViewLoc = glGetUniformLocation(shader, "invProjectionView");

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

	}

	void SSRTraceRP::execute(RenderDatabase &db) {

		glBindFramebuffer(GL_FRAMEBUFFER, db.effectBuffer.ssrTraceBuffer.frameBuffer);
		glUseProgram(shader);

		glUniform1i(renderMap1UniformLoc, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, db.gBuffer.renderBuffer1);

		glUniform1i(renderMap2UniformLoc, 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, db.gBuffer.renderBuffer2);

		glUniform1i(renderMap3UniformLoc, 2);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, db.gBuffer.renderBuffer3);

		glUniform1i(depthMapLoc, 3);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, db.gBuffer.depthBuffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

		glUniform2f(screenDimensionLoc, db.targetWidthPx, db.targetHeightPx);
		glUniform3f(cameraPositionLoc, db.camera.position.x, db.camera.position.y, db.camera.position.z);
		glUniform1f(cameraZNearLoc, db.camera.perspective.zNear);
		glUniform1f(cameraZFarLoc, db.camera.perspective.zFar);
		
		Mat4 viewMat = mat4View(db.camera.position, db.camera.position +
			db.camera.direction, db.camera.up);
		Mat4 projectionViewMat = db.camera.projection * viewMat;
		Mat4 invProjectionViewMat = mat4Inverse(projectionViewMat);
		glUniformMatrix4fv(invProjectionViewLoc, 1, GL_TRUE, (const GLfloat*)invProjectionViewMat.elem);

		glViewport(0, 0, db.targetWidthPx, db.targetHeightPx);
		glBindVertexArray(db.quadVAO);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUseProgram(0);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

	}

	void SSRTraceRP::shutdown(RenderDatabase &database) {
		glDeleteProgram(shader);
	}

}