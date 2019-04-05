//
// Created by Kevin Yudi Utama on 29/8/18.
//

#include "render/data.h"
#include "render/intern/asset.h"
#include "render/intern/glext.h"
#include "core/math.h"

namespace Soul {
	namespace Render {

		void WireframeRP::init(Database& database) {

			program = GLExt::ProgramCreate(RenderAsset::ShaderFile::wireframe);


			GLuint sceneDataBlockIndex = glGetUniformBlockIndex(program, Constant::CAMERA_DATA_NAME);
			glUniformBlockBinding(program, sceneDataBlockIndex, Constant::CAMERA_DATA_BINDING_POINT);

			modelUniformLoc = glGetUniformLocation(program, "model");

		}

		void WireframeRP::execute(Database &db) {
			SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

			Camera& camera = db.camera;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, db.targetWidthPx, db.targetHeightPx);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glDepthFunc(GL_ALWAYS);
			glUseProgram(program);

			for (int i = 0; i < db.wireframeMeshes.size(); i++) {
				const Mesh* mesh = db.wireframeMeshes[i];
				const Material& material = db.materialBuffer[mesh->materialID];

				glUniformMatrix4fv(modelUniformLoc, 1, GL_TRUE, (const GLfloat*)mesh->transform.elem);
				GLExt::ErrorCheck("WireframePP::execute");
				glBindVertexArray(mesh->vaoHandle);
				GLExt::ErrorCheck("WireframePP::execute");
				glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
				GLExt::ErrorCheck("WireframePP::execute");
			}
			
			glDepthFunc(GL_LEQUAL);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glBindVertexArray(0);
			glUseProgram(0);
			
			GLExt::ErrorCheck("WireframePP::execute");

			SOUL_PROFILE_RANGE_POP();
		}

		void WireframeRP::shutdown(Database &database) {
			GLExt::ProgramDelete(&program);
		}

	}
}