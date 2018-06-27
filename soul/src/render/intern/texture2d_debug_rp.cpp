#include "render/type.h"
#include "render/intern/asset.h"
#include "render/intern/util.h"

namespace Soul {

    void Texture2DDebugRP::init(RenderDatabase& database) {

		shader = RenderUtil::GLProgramCreate(RenderAsset::ShaderFile::texture2dDebug);
 
    }

    void Texture2DDebugRP::execute(RenderDatabase& database) {

        Camera& camera = database.camera;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        float fracSize = 1.0f / 4;
        glViewport(0, 0, database.targetWidthPx * fracSize, database.targetHeightPx * fracSize);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glUseProgram(shader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, database.gBuffer.renderBuffer3);
		glBindVertexArray(database.quadVAO);
		glDisable(GL_DEPTH_TEST);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);
    }

    void Texture2DDebugRP::shutdown(RenderDatabase &database) {
        glDeleteProgram(shader);
    }


}