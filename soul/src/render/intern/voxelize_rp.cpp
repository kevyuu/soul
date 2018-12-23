#include "core/debug.h"
#include "core/math.h"

#include "render/type.h"
#include "render/intern/asset.h"
#include "render/intern/glext.h"

namespace Soul {
	void VoxelizeRP::init(RenderDatabase& database) {
		program = GLExt::ProgramCreate(RenderAsset::ShaderFile::voxelize);

		projectionLoc = glGetUniformLocation(program, "projection");
		viewLoc = glGetUniformLocation(program, "view");
		modelLoc = glGetUniformLocation(program, "model");
		albedoMapLoc = glGetUniformLocation(program, "material.albedoMap");
		normalMapLoc = glGetUniformLocation(program, "material.normalMap");
		metallicMapLoc = glGetUniformLocation(program, "material.metallicMapLoc");
		roughnessMapLoc = glGetUniformLocation(program, "material. roughnessMapLoc");
		voxelAlbedoBufferLoc = glGetUniformLocation(program, "voxelAlbedoBuffer");
		voxelNormalBufferLoc = glGetUniformLocation(program, "voxelNormalBuffer");
		voxelFrustumCenterLoc = glGetUniformLocation(program, "voxelFrustumCenter");
		voxelFrustumResoLoc = glGetUniformLocation(program, "voxelFrustumReso");
		voxelFrustumHalfSpanLoc = glGetUniformLocation(program, "voxelFrustumHalfSpan");

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "OpenGL Error");

	}

	void VoxelizeRP::execute(RenderDatabase& db) {


		float data[4] = { 0.0f, 0.0f, 0.0f, 0.0f};

		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glClearTexImage(db.voxelGIBuffer.lightVoxelTex, 0, GL_RGBA, GL_FLOAT, data);
		glClearTexImage(db.voxelGIBuffer.gVoxelAlbedoTex, 0, GL_RGBA, GL_FLOAT, data);
		glClearTexImage(db.voxelGIBuffer.gVoxelNormalTex, 0, GL_RGBA, GL_FLOAT, data);

		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		glUseProgram(program);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

		Mat4 projection = mat4Ortho(
			-db.voxelFrustumHalfSpan, db.voxelFrustumHalfSpan,
			-db.voxelFrustumHalfSpan, db.voxelFrustumHalfSpan,
			-db.voxelFrustumHalfSpan, db.voxelFrustumHalfSpan);
		glUniformMatrix4fv(projectionLoc, 1, GL_TRUE, (GLfloat*) projection.elem);
		Mat4 view = mat4View(db.voxelFrustumCenter, db.voxelFrustumCenter + Vec3f(0, 0, 1.0f), Vec3f(0, 1.0f, 0.0f));
		glUniformMatrix4fv(viewLoc, 1, GL_TRUE, (GLfloat*)view.elem);
		glUniform3fv(voxelFrustumCenterLoc, 1, (GLfloat*)&db.voxelFrustumCenter);
		glUniform1i(voxelFrustumResoLoc, db.voxelFrustumReso);
		glUniform1f(voxelFrustumHalfSpanLoc, db.voxelFrustumHalfSpan);

		glUniform1i(voxelAlbedoBufferLoc, 6);
		glBindImageTexture(6, db.voxelGIBuffer.gVoxelAlbedoTex, 0, true, 0, GL_READ_WRITE, GL_R32UI);

		glUniform1i(voxelNormalBufferLoc, 7);
		glBindImageTexture(7, db.voxelGIBuffer.gVoxelNormalTex, 0, true, 0, GL_READ_WRITE, GL_R32UI);

		glViewport(0, 0, db.voxelFrustumReso, db.voxelFrustumReso);
		glDisable(GL_DEPTH_TEST);

		for (int i = 0; i < db.meshBuffer.getSize(); i++) {

			const Mesh& mesh = db.meshBuffer.get(i);
			const Material& material = db.materialBuffer.get(mesh.materialID);

			glUniformMatrix4fv(modelLoc, 1, GL_TRUE, (const GLfloat*)mesh.transform.elem);

			glUniform1i(albedoMapLoc, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, material.albedoMap);

			glUniform1i(normalMapLoc, 1);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, material.normalMap);

			glUniform1i(metallicMapLoc, 2);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, material.metallicMap);

			glUniform1i(roughnessMapLoc, 3);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, material.roughnessMap);

			SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

			glBindVertexArray(mesh.vaoHandle);
			glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
			
		}

		glUseProgram(0);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

	}

	void VoxelizeRP::shutdown(RenderDatabase& database) {
		glDeleteProgram(0);
	}
}