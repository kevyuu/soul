#include "render/type.h"
#include "render/intern/util.h"
#include "render/intern/asset.h"
#include "core/math.h"

namespace Soul {
    void SkyboxRP::init(RenderDatabase& database) {

        shader = RenderUtil::GLProgramCreate(RenderAsset::ShaderFile::skybox);

        projectionLoc = glGetUniformLocation(shader, "projection");
        viewLoc = glGetUniformLocation(shader, "view");
        skyboxLoc = glGetUniformLocation(shader, "skybox");

    }

    void SkyboxRP::execute(RenderDatabase& database) {

        glUseProgram(shader);

        glUniform1i(skyboxLoc, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, database.environment.cubemap);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, (const GLfloat*) database.sceneDataUBO.view.elem);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, (const GLfloat*) database.sceneDataUBO.projection.elem);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glBindVertexArray(database.cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glUseProgram(0);
    }

    void SkyboxRP::shutdown(RenderDatabase& database) {
        glDeleteProgram(shader);
    }
}