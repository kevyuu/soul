#include "render/type.h"
#include "asset.h"
#include "glext.h"

namespace Soul {
    void PBRSceneRP::init(RenderDatabase& db) {

        predepthShader = GLExt::ProgramCreate(RenderAsset::ShaderFile::predepth);
        GLuint sceneDataBlockIndexPredepth = glGetUniformBlockIndex(predepthShader, "SceneData");
        glUniformBlockBinding(predepthShader, sceneDataBlockIndexPredepth, RenderConstant::SCENE_DATA_BINDING_POINT);
        predepthModelUniformLoc = glGetUniformLocation(predepthShader, "model");


        sceneShader = GLExt::ProgramCreate(RenderAsset::ShaderFile::pbr);
        GLuint sceneDataBlockIndex = glGetUniformBlockIndex(sceneShader, "SceneData");
        glUniformBlockBinding(sceneShader, sceneDataBlockIndex, RenderConstant::SCENE_DATA_BINDING_POINT);

        GLuint lightDataBlockIndex = glGetUniformBlockIndex(sceneShader, "LightData");
        glUniformBlockBinding(sceneShader, lightDataBlockIndex, RenderConstant::LIGHT_DATA_BINDING_POINT);

        modelUniformLoc = glGetUniformLocation(sceneShader, "model");
        viewPosUniformLoc = glGetUniformLocation(sceneShader, "viewPosition");
        albedoMapPositionLoc = glGetUniformLocation(sceneShader, "material.albedoMap");
        normalMapPositionLoc = glGetUniformLocation(sceneShader, "material.normalMap");
        metallicMapPositionLoc = glGetUniformLocation(sceneShader, "material.metallicMap");
        roughnessMapPositionLoc = glGetUniformLocation(sceneShader, "material.roughnessMap");

        ambientEnergyLoc = glGetUniformLocation(sceneShader, "environment.ambientEnergy");
        ambientColorLoc = glGetUniformLocation(sceneShader, "environment.ambientColor");

        shadowMapLoc = glGetUniformLocation(sceneShader, "shadowMap");
        brdfMapLoc = glGetUniformLocation(sceneShader, "brdfMap");
        diffuseMapLoc = glGetUniformLocation(sceneShader, "diffuseMap");
        specularMapLoc = glGetUniformLocation(sceneShader, "specularMap");

    }

    void PBRSceneRP::execute(RenderDatabase &database) {

        Camera& camera = database.camera;

        glViewport(0, 0, database.targetWidthPx, database.targetHeightPx);

        glUseProgram(predepthShader);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearDepth(1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);

        for (int i = 0; i < database.meshBuffer.getSize(); i++) {
            const Mesh& mesh = database.meshBuffer.get(i);

            glUniformMatrix4fv(predepthModelUniformLoc, 1, GL_TRUE, (const GLfloat*) mesh.transform.elem);

            glBindVertexArray(mesh.vaoHandle);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
        }

        glUseProgram(sceneShader);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClear(GL_COLOR_BUFFER_BIT);
        glDepthFunc(GL_LEQUAL);

        glUniform1i(shadowMapLoc, 5);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, database.shadowAtlas.texHandle);

        glUniform1i(brdfMapLoc, 6);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, database.environment.brdfMap);

        glUniform1i(diffuseMapLoc, 7);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_CUBE_MAP, database.environment.diffuseMap);

        glUniform1i(specularMapLoc, 8);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_CUBE_MAP, database.environment.specularMap);

        glUniform1f(ambientEnergyLoc, database.environment.ambientEnergy);
        Vec3f ambientColor = database.environment.ambientColor;
        glUniform3f(ambientColorLoc, ambientColor.x, ambientColor.y, ambientColor.z);


        for (int i = 0; i < database.meshBuffer.getSize(); i++) {

            const Mesh& mesh = database.meshBuffer.get(i);
            const Material& material = database.materialBuffer.get(mesh.materialID);

            glUniformMatrix4fv(modelUniformLoc, 1, GL_TRUE, (const GLfloat*) mesh.transform.elem);

            glUniform3fv(viewPosUniformLoc, 1, (GLfloat*) &camera.position);

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

            glBindVertexArray(mesh.vaoHandle);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
        }

        glBindVertexArray(0);
        glUseProgram(0);
    }

    void PBRSceneRP::shutdown(RenderDatabase &database) {
        glDeleteProgram(predepthShader);
        glDeleteProgram(sceneShader);
    }
}