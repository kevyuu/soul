//
// Created by Kevin Yudi Utama on 14/8/18.
//

#include "render/type.h"
#include "render/intern/asset.h"
#include "render/intern/util.h"

namespace Soul {
    void BRDFMapRP::init(RenderDatabase &database) {

        shader = RenderUtil::GLProgramCreate(RenderAsset::ShaderFile::brdfMap);

        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glGenRenderbuffers(1, &renderBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void BRDFMapRP::execute(RenderDatabase& database) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, database.environment.brdfMap, 0);

        glUseProgram(shader);

        glViewport(0, 0, 512, 512);
        glBindVertexArray(database.quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(0);

		RenderUtil::GLErrorCheck("BRDFMapRP::execute");

    }

    void BRDFMapRP::shutdown(RenderDatabase &database) {
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteProgram(shader);
    }
}