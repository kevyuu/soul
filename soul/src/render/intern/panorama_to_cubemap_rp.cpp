#include "../type.h"
#include "util.h"
#include "asset.h"
#include "core/math.h"

namespace Soul {
    void PanoramaToCubemapRP::init(RenderDatabase &database) {

        shader = RenderUtil::GLProgramCreate(RenderAsset::ShaderFile::panoramaToCubemap);

        projectionLoc = glGetUniformLocation(shader, "projection");
        viewLoc = glGetUniformLocation(shader, "view");

        glGenFramebuffers(1, &renderTarget);
        glBindFramebuffer(GL_FRAMEBUFFER, renderTarget);
        glGenRenderbuffers(1, &renderBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    }

    void PanoramaToCubemapRP::execute(RenderDatabase &database) {
        static const Mat4 projection = mat4Perspective(PI / 2, 1, 0.1, 10.0);
        static const Mat4 captureViews[] = {
            mat4View(Vec3f(0, 0, 0), Vec3f(1.0f, 0.0f, 0.0f), Vec3f(0.0f, -1.0f, 0.0f)),
            mat4View(Vec3f(0, 0, 0), Vec3f(-1.0f, 0.0f, 0.0f), Vec3f(0.0f, -1.0f, 0.0f)),
            mat4View(Vec3f(0, 0, 0), Vec3f(0.0f, 1.0f, 0.0f), Vec3f(0.0f, 0.0f, 1.0f)),
            mat4View(Vec3f(0, 0, 0), Vec3f(0.0f, -1.0f, 0.0f), Vec3f(0.0f, 0.0f, -1.0f)),
            mat4View(Vec3f(0, 0, 0), Vec3f(0.0f, 0.0f, 1.0f), Vec3f(0.0f, -1.0f, 0.0f)),
            mat4View(Vec3f(0, 0, 0), Vec3f(0.0f, 0.0f, -1.0f), Vec3f(0.0f, -1.0f, 0.0f))
        };
        
        glBindFramebuffer(GL_FRAMEBUFFER, renderTarget);
        glBindVertexArray(database.cubeVAO);
        glUseProgram(shader);
        glViewport(0, 0, 512, 512);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, database.environment.panorama);
        glUniformMatrix4fv(projectionLoc, 1, GL_TRUE, (const GLfloat*) projection.elem);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        for (int i = 0; i < 6; i++) {
            glUniformMatrix4fv(viewLoc, 1, GL_TRUE, (const GLfloat*) captureViews[i].elem);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                    database.environment.cubemap, 0);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(0);
    }

    void PanoramaToCubemapRP::shutdown(RenderDatabase &database) {
        glDeleteProgram(shader);
        glDeleteFramebuffers(1, &renderTarget);
    }

}