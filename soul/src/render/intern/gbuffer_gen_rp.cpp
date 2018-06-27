//
// Created by Kevin Yudi Utama on 29/8/18.
//

#include "render/type.h"
#include "render/intern/asset.h"
#include "render/intern/util.h"

namespace Soul {

    void GBufferGenRP::init(RenderDatabase& database) {

        predepthShader = RenderUtil::GLProgramCreate(RenderAsset::ShaderFile::predepth);

        GLuint sceneDataBlockIndexPredepth = glGetUniformBlockIndex(predepthShader, "SceneData");

        glUniformBlockBinding(predepthShader, sceneDataBlockIndexPredepth, RenderConstant::SCENE_DATA_BINDING_POINT);
        predepthModelUniformLoc = glGetUniformLocation(predepthShader, "model");


        gBufferShader = RenderUtil::GLProgramCreate(RenderAsset::ShaderFile::gbufferGen);

        GLuint sceneDataBlockIndex = glGetUniformBlockIndex(gBufferShader, "SceneData");
        glUniformBlockBinding(gBufferShader, sceneDataBlockIndex, RenderConstant::SCENE_DATA_BINDING_POINT);
		GLuint lightDataBlockIndex = glGetUniformBlockIndex(gBufferShader, "LightData");
		glUniformBlockBinding(gBufferShader, lightDataBlockIndex, RenderConstant::LIGHT_DATA_BINDING_POINT);

        modelUniformLoc = glGetUniformLocation(gBufferShader, "model");
        albedoMapPositionLoc = glGetUniformLocation(gBufferShader, "material.albedoMap");
        normalMapPositionLoc = glGetUniformLocation(gBufferShader, "material.normalMap");
        metallicMapPositionLoc = glGetUniformLocation(gBufferShader, "material.metallicMap");
        roughnessMapPositionLoc = glGetUniformLocation(gBufferShader, "material.roughnessMap");
        aoMapPositionLoc = glGetUniformLocation(gBufferShader, "material.aoMap");
		shadowMapLoc = glGetUniformLocation(gBufferShader, "shadowMap");
		viewPositionLoc = glGetUniformLocation(gBufferShader, "viewPosition");

    }

    void GBufferGenRP::execute(RenderDatabase &db) {

        Camera& camera = db.camera;

        glBindFramebuffer(GL_FRAMEBUFFER, db.gBuffer.frameBuffer);

        glUseProgram(predepthShader);
        glDisable(GL_BLEND);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_TRUE);
		glClearDepth(1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDrawBuffers(0, NULL);

		glViewport(0, 0, db.targetWidthPx, db.targetHeightPx);
		for (int i = 0; i < db.meshBuffer.getSize(); i++) {
            const Mesh& mesh = db.meshBuffer.get(i);
            glUniformMatrix4fv(predepthModelUniformLoc, 1, GL_TRUE, (const GLfloat*) mesh.transform.elem);
            glBindVertexArray(mesh.vaoHandle);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
        }

        unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(4, attachments);

        glUseProgram(gBufferShader);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glClear(GL_COLOR_BUFFER_BIT);
        glDepthFunc(GL_LEQUAL);

        glUniform1i(shadowMapLoc, 5);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, db.shadowAtlas.texHandle);

		glUniform3f(viewPositionLoc, db.camera.position.x, db.camera.position.y, db.camera.position.z);

		glViewport(0, 0, db.targetWidthPx, db.targetHeightPx);

        for (int i = 0; i < db.meshBuffer.getSize(); i++) {

            const Mesh& mesh = db.meshBuffer.get(i);
            const Material& material = db.materialBuffer.get(mesh.materialID);

            glUniformMatrix4fv(modelUniformLoc, 1, GL_TRUE, (const GLfloat*) mesh.transform.elem);

            glUniform1i(albedoMapPositionLoc, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, material.albedoMap);

            glUniform1i(normalMapPositionLoc, 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, material.normalMap);

            glUniform1i(metallicMapPositionLoc, 2);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, material.metallicMap);

            glUniform1i(roughnessMapPositionLoc, 3);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, material.roughnessMap);

            glUniform1i(aoMapPositionLoc, 4);
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, material.aoMap);

            glBindVertexArray(mesh.vaoHandle);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);
        glDepthMask(GL_TRUE);

		RenderUtil::GLErrorCheck("GBufferGenRPP::execute");

    }

    void GBufferGenRP::shutdown(RenderDatabase &database) {
        glDeleteProgram(gBufferShader);
        glDeleteProgram(predepthShader);
    }

}