//
// Created by Kevin Yudi Utama on 29/8/18.
//

#include "render/data.h"
#include "render/intern/asset.h"
#include "render/intern/glext.h"
#include "core/math.h"

namespace Soul {
	namespace Render {

		void GBufferGenRP::init(Database& database) {

			predepthProgram = GLExt::ProgramCreate(RenderAsset::ShaderFile::predepth);

			GLuint sceneDataBlockIndexPredepth = glGetUniformBlockIndex(predepthProgram, Constant::CAMERA_DATA_NAME);
			glUniformBlockBinding(predepthProgram, sceneDataBlockIndexPredepth, Constant::CAMERA_DATA_BINDING_POINT);

			predepthModelUniformLoc = glGetUniformLocation(predepthProgram, "model");

			gBufferGenProgram = GLExt::ProgramCreate(RenderAsset::ShaderFile::gbufferGen);

			GLuint sceneDataBlockIndex = glGetUniformBlockIndex(gBufferGenProgram, Constant::CAMERA_DATA_NAME);
			glUniformBlockBinding(gBufferGenProgram, sceneDataBlockIndex, Constant::CAMERA_DATA_BINDING_POINT);
			GLuint lightDataBlockIndex = glGetUniformBlockIndex(gBufferGenProgram, Constant::LIGHT_DATA_NAME);
			glUniformBlockBinding(gBufferGenProgram, lightDataBlockIndex, Constant::LIGHT_DATA_BINDING_POINT);

			modelUniformLoc = glGetUniformLocation(gBufferGenProgram, "model");

			albedoMapLoc = glGetUniformLocation(gBufferGenProgram, "material.albedoMap");
			normalMapLoc = glGetUniformLocation(gBufferGenProgram, "material.normalMap");
			metallicMapLoc = glGetUniformLocation(gBufferGenProgram, "material.metallicMap");
			roughnessMapLoc = glGetUniformLocation(gBufferGenProgram, "material.roughnessMap");
			aoMapLoc = glGetUniformLocation(gBufferGenProgram, "material.aoMap");

			materialFlagsLoc = glGetUniformLocation(gBufferGenProgram, "material.flags");

			albedoLoc = glGetUniformLocation(gBufferGenProgram, "material.albedo");
			metallicLoc = glGetUniformLocation(gBufferGenProgram, "material.metallic");
			roughnessLoc = glGetUniformLocation(gBufferGenProgram, "material.roughness");

			shadowMapLoc = glGetUniformLocation(gBufferGenProgram, "shadowMap");
			viewPositionLoc = glGetUniformLocation(gBufferGenProgram, "viewPosition");
			ambientFactorLoc = glGetUniformLocation(gBufferGenProgram, "ambientFactor");

		}

		void GBufferGenRP::execute(Database &db) {
			SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

			Camera& camera = db.camera;

			glBindFramebuffer(GL_FRAMEBUFFER, db.gBuffer.frameBuffer);

			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace(GL_CCW);

			glUseProgram(predepthProgram);
			glDisable(GL_BLEND);
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			glDepthMask(GL_TRUE);
			glClearDepth(1.0f);
			glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glDrawBuffers(0, NULL);

			glViewport(0, 0, db.targetWidthPx, db.targetHeightPx);
			for (int i = 0; i < db.meshBuffer.count(); i++) {
				const Mesh& mesh = db.meshBuffer.get(i);
				glUniformMatrix4fv(predepthModelUniformLoc, 1, GL_TRUE, (const GLfloat*)mesh.transform.elem);
				glBindVertexArray(mesh.vaoHandle);
				glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
			}

			unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers(4, attachments);

			glUseProgram(gBufferGenProgram);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glClear(GL_COLOR_BUFFER_BIT);
			glDepthFunc(GL_LEQUAL);

			glUniform1i(shadowMapLoc, 5);
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, db.shadowAtlas.texHandle);

			glUniform3f(viewPositionLoc, db.camera.position.x, db.camera.position.y, db.camera.position.z);
			Vec3f ambientFactor = db.environment.ambientColor * db.environment.ambientEnergy;
			glUniform3f(ambientFactorLoc, ambientFactor.x, ambientFactor.y, ambientFactor.z);


			glViewport(0, 0, db.targetWidthPx, db.targetHeightPx);



			for (int i = 0; i < db.meshBuffer.count(); i++) {

				const Mesh& mesh = db.meshBuffer[i];
				const Material& material = db.materialBuffer[mesh.materialID];

				glUniformMatrix4fv(modelUniformLoc, 1, GL_TRUE, (const GLfloat*)mesh.transform.elem);

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

				glUniform1i(aoMapLoc, 4);
				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, material.aoMap);

				glUniform1ui(materialFlagsLoc, material.flags);

				glUniform3f(albedoLoc, material.albedo.x, material.albedo.y, material.albedo.z);
				glUniform1f(roughnessLoc, material.roughness);
				glUniform1f(metallicLoc, material.metallic);

				glBindVertexArray(mesh.vaoHandle);
				glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);

			}

			glDisable(GL_CULL_FACE);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glBindVertexArray(0);
			glUseProgram(0);
			glDepthMask(GL_TRUE);

			GLExt::ErrorCheck("GBufferGenRPP::execute");

			SOUL_PROFILE_RANGE_POP();
		}

		void GBufferGenRP::shutdown(Database &database) {
			GLExt::ProgramDelete(&gBufferGenProgram);
			GLExt::ProgramDelete(&predepthProgram);
		}

	}
}