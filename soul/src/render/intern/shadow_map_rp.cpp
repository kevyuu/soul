#include "render/intern/asset.h"
#include "render/type.h"
#include "render/intern/glext.h"
#include "core/debug.h"

namespace Soul {

    void ShadowMapRP::init(RenderDatabase &database) {

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass());
        shader = GLExt::ProgramCreate(RenderAsset::ShaderFile::shadowMap);

        modelLoc = glGetUniformLocation(shader, "model");
        shadowMatrixLoc = glGetUniformLocation(shader, "shadowMatrix");

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass());

    }

    void ShadowMapRP::execute(RenderDatabase &database) {
		SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

        int resolution = database.shadowAtlas.resolution;

        glViewport(0, 0, resolution, resolution);
        glBindFramebuffer(GL_FRAMEBUFFER, database.shadowAtlas.framebuffer);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDisable(GL_DITHER);
		glDepthMask(GL_TRUE);
		glColorMask(0, 0, 0, 0);
		glClearDepth(1.0f);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_SCISSOR_TEST);
		
        glUseProgram(shader);


        for (int i = 0; i < database.dirLightCount; i++) {
            const DirectionalLight& light = database.dirLights[i];
            int quadrant = light.shadowKey.quadrant;
            int subdiv = light.shadowKey.subdiv;
            int subdivCount = database.shadowAtlas.subdivSqrtCount[quadrant] * database.shadowAtlas.subdivSqrtCount[quadrant];
            int atlasReso = database.shadowAtlas.resolution;
            int subdivReso = atlasReso / (2 * database.shadowAtlas.subdivSqrtCount[quadrant]);
            int xSubdiv = subdiv % database.shadowAtlas.subdivSqrtCount[quadrant];
            int ySubdiv = subdiv / database.shadowAtlas.subdivSqrtCount[quadrant];

            float bottomSubdivViewport = (quadrant / 2) * 0.5f * atlasReso + ySubdiv * subdivReso;
            float leftSubdivViewport = (quadrant % 2) * 0.5f * atlasReso + xSubdiv * subdivReso;

            for (int j = 0; j < 4; j++) {

                int splitReso = subdivReso / 2;
                int xSplit = j % 2;
                int ySplit = j / 2;

                float bottomSplitViewport = bottomSubdivViewport + (ySplit * splitReso);
                float leftSplitViewport = leftSubdivViewport + (xSplit * splitReso);

                for (int k = 0; k < database.meshBuffer.getSize(); k++) {
                    const Mesh& mesh = database.meshBuffer.get(k);
                    glUniformMatrix4fv(modelLoc, 1, GL_TRUE, (const GLfloat*)mesh.transform.elem);
                    glUniformMatrix4fv(shadowMatrixLoc, 1, GL_TRUE, (const GLfloat*)light.shadowMatrix[j].elem);
                    glBindVertexArray(mesh.vaoHandle);
                    glScissor(leftSplitViewport, bottomSplitViewport, splitReso, splitReso);
                    glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
                }
            }
        }

        glDisable(GL_SCISSOR_TEST);
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(0);

		GLExt::ErrorCheck("ShadowMapRP::execute");
		
		SOUL_PROFILE_RANGE_POP();
    }

    void ShadowMapRP::shutdown(RenderDatabase &database) {
        glDeleteProgram(shader);
    }


}