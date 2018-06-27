#include <render/type.h>
#include "render/type.h"
#include "render/intern/asset.h"
#include "render/intern/util.h"
#include "core/math.h"

namespace Soul {

	void SSRResolveRP::init(RenderDatabase& database) {
		shader = RenderUtil::GLProgramCreate(RenderAsset::ShaderFile::ssrResolve);

		reflectionPosBufferLoc = glGetUniformLocation(shader, "reflectionPosBuffer");
		lightBufferLoc = glGetUniformLocation(shader, "lightBuffer");
		renderMap1Loc = glGetUniformLocation(shader, "renderMap1");
		renderMap2Loc = glGetUniformLocation(shader, "renderMap2");
		renderMap3Loc = glGetUniformLocation(shader, "renderMap3");
		renderMap4Loc = glGetUniformLocation(shader, "renderMap4");
		depthMapLoc = glGetUniformLocation(shader, "depthMap");
		FGMapLoc = glGetUniformLocation(shader, "FGMap");
		voxelLightBufferLoc = glGetUniformLocation(shader, "voxelLightBuffer");

		screenDimensionLoc = glGetUniformLocation(shader, "screenDimension");
		cameraPositionLoc = glGetUniformLocation(shader, "cameraPosition");
		invProjectionViewLoc = glGetUniformLocation(shader, "invProjectionView");

		voxelFrustumResoLoc = glGetUniformLocation(shader, "voxelFrustumReso");
		voxelFrustumHalfSpanLoc = glGetUniformLocation(shader, "voxelFrustumHalfSpan");
		voxelFrustumCenterLoc = glGetUniformLocation(shader, "voxelFrustumCenter");

		RenderUtil::GLErrorCheck("SSRResolveRP::init");
	}

	void SSRResolveRP::execute(RenderDatabase &db) {

		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glBindFramebuffer(GL_FRAMEBUFFER, db.lightBuffer.frameBuffer);
		glUseProgram(shader);

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

		glUniform2f(screenDimensionLoc, db.targetWidthPx, db.targetHeightPx);
		glUniform3f(cameraPositionLoc, db.camera.position.x,
					db.camera.position.y, db.camera.position.z);

		glUniform1i(voxelFrustumResoLoc, db.voxelFrustumReso);
		glUniform1f(voxelFrustumHalfSpanLoc, db.voxelFrustumHalfSpan);
		glUniform3fv(voxelFrustumCenterLoc, 1, (GLfloat*)&db.voxelFrustumCenter);


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

		glBindFramebuffer(GL_READ_FRAMEBUFFER, db.lightBuffer.frameBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, db.targetWidthPx, db.targetHeightPx, 0, 0, db.targetWidthPx, db.targetHeightPx, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUseProgram(0);

		RenderUtil::GLErrorCheck("SSRResolveRP::execute");

	}

	void SSRResolveRP::shutdown(RenderDatabase &database) {
		glDeleteProgram(shader);
	}

}