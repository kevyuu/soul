#include "core/debug.h"
#include "core/math.h"

#include "render/data.h"
#include "render/intern/asset.h"
#include "render/intern/glext.h"

namespace Soul {
	namespace Render {
		void VoxelizeRP::init(Database& database) {
			program = GLExt::ProgramCreate(RenderAsset::ShaderFile::voxelize);

			GLExt::UBOBind(program, Constant::VOXEL_GI_DATA_NAME, Constant::VOXEL_GI_DATA_BINDING_POINT);

			projectionViewLoc[0] = glGetUniformLocation(program, "projectionView[0]");
			projectionViewLoc[1] = glGetUniformLocation(program, "projectionView[1]");
			projectionViewLoc[2] = glGetUniformLocation(program, "projectionView[2]");

			inverseProjectionViewLoc[0] = glGetUniformLocation(program, "inverseProjectionView[0]");
			inverseProjectionViewLoc[1] = glGetUniformLocation(program, "inverseProjectionView[1]");
			inverseProjectionViewLoc[2] = glGetUniformLocation(program, "inverseProjectionView[2]");

			modelLoc = glGetUniformLocation(program, "model");

			albedoMapLoc = glGetUniformLocation(program, "material.albedoMap");
			normalMapLoc = glGetUniformLocation(program, "material.normalMap");
			metallicMapLoc = glGetUniformLocation(program, "material.metallicMap");
			roughnessMapLoc = glGetUniformLocation(program, "material.roughnessMap");
			aoMapLoc = glGetUniformLocation(program, "material.aoMap");
			emissiveMapLoc = glGetUniformLocation(program, "material.emissiveMap");

			materialFlagsLoc = glGetUniformLocation(program, "material.flags");

			albedoLoc = glGetUniformLocation(program, "material.albedo");
			metallicLoc = glGetUniformLocation(program, "material.metallic");
			roughnessLoc = glGetUniformLocation(program, "material.roughness");
			emissiveLoc = glGetUniformLocation(program, "material.emissive");
			
			voxelAlbedoBufferLoc = glGetUniformLocation(program, "voxelAlbedoBuffer");
			voxelNormalBufferLoc = glGetUniformLocation(program, "voxelNormalBuffer");
			voxelEmissiveBufferLoc = glGetUniformLocation(program, "voxelEmissiveBuffer");

			SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "OpenGL Error");

		}

		void VoxelizeRP::execute(Database& db) {

			SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

			float data[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			glClearTexImage(db.voxelGIBuffer.lightVoxelTex, 0, GL_RGBA, GL_FLOAT, data);
			glClearTexImage(db.voxelGIBuffer.gVoxelAlbedoTex, 0, GL_RGBA, GL_FLOAT, data);
			glClearTexImage(db.voxelGIBuffer.gVoxelNormalTex, 0, GL_RGBA, GL_FLOAT, data);

			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			glUseProgram(program);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

			float voxelFrustumHalfSpan = db.voxelGIConfig.halfSpan;
			Vec3f voxelFrustumCenter = db.voxelGIConfig.center;

			Mat4 projection = mat4Ortho(
				-voxelFrustumHalfSpan, voxelFrustumHalfSpan,
				-voxelFrustumHalfSpan, voxelFrustumHalfSpan,
				-voxelFrustumHalfSpan, voxelFrustumHalfSpan);

			Mat4 view[3];
			view[0] = mat4View(voxelFrustumCenter, voxelFrustumCenter + Vec3f(1.0f, 0.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f));
			view[1] = mat4View(voxelFrustumCenter, voxelFrustumCenter + Vec3f(0.0f, 1.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f));
			view[2] = mat4View(voxelFrustumCenter, voxelFrustumCenter + Vec3f(0.0f, 0.0f, 1.0f), Vec3f(0.0f, 1.0f, 0.0f));

			Mat4 projectionView[3];
			for (int i = 0; i < 3; i++) {
				projectionView[i] = projection * view[i];
			}

			Mat4 inverseProjectionView[3];
			for (int i = 0; i < 3; i++) {
				inverseProjectionView[i] = mat4Inverse(projectionView[i]);
			}

			for (int i = 0; i < 3; i++) {
				glUniformMatrix4fv(projectionViewLoc[i], 1, GL_TRUE, (GLfloat*)projectionView[i].elem);
			}

			for (int i = 0; i < 3; i++) {
				glUniformMatrix4fv(inverseProjectionViewLoc[i], 1, GL_TRUE, (GLfloat*)inverseProjectionView[i].elem);
			}

			int voxelFrustumReso = db.voxelGIConfig.resolution;

			glUniform1i(voxelAlbedoBufferLoc, 3);
			glBindImageTexture(3, db.voxelGIBuffer.gVoxelAlbedoTex, 0, true, 0, GL_READ_WRITE, GL_R32UI);

			glUniform1i(voxelEmissiveBufferLoc, 4);
			glBindImageTexture(4, db.voxelGIBuffer.gVoxelEmissiveTex, 0, true, 0, GL_READ_WRITE, GL_R32UI);

			SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

			glUniform1i(voxelNormalBufferLoc, 5);
			glBindImageTexture(5, db.voxelGIBuffer.gVoxelNormalTex, 0, true, 0, GL_READ_WRITE, GL_R32UI);

			SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

			glViewport(0, 0, voxelFrustumReso, voxelFrustumReso);
			glDisable(GL_DEPTH_TEST);

			for (int i = 0; i < db.meshBuffer.size(); i++) {

				const Mesh& mesh = db.meshBuffer.get(i);
				const Material& material = db.materialBuffer.get(mesh.materialID);

				glUniformMatrix4fv(modelLoc, 1, GL_TRUE, (const GLfloat*)mesh.transform.elem);

				glUniform1i(albedoMapLoc, 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, material.albedoMap);

				glUniform1i(normalMapLoc, 1);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, material.normalMap);

				glUniform1i(emissiveMapLoc, 2);
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, material.emissiveMap);

				glUniform1ui(materialFlagsLoc, material.flags);

				glUniform3f(albedoLoc, material.albedo.x, material.albedo.y, material.albedo.z);
				glUniform3f(emissiveLoc, material.emissive.x, material.emissive.y, material.emissive.z);
				glUniform1f(roughnessLoc, material.roughness);
				glUniform1f(metallicLoc, material.metallic);


				SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

				glBindVertexArray(mesh.vaoHandle);
				glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);

			}

			glUseProgram(0);

			SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");
			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			SOUL_PROFILE_RANGE_POP();
		}

		void VoxelizeRP::shutdown(Database& database) {
			glDeleteProgram(0);
		}
	}
}