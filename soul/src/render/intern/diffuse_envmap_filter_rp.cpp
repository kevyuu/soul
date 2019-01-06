#include "render/type.h"
#include "core/math.h"
#include "glext.h"
#include "asset.h"

namespace Soul {
    void DiffuseEnvmapFilterRP::init(RenderDatabase &database) {
        shader = GLExt::ProgramCreate(RenderAsset::ShaderFile::diffuseEnvmapFilter);

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

    void DiffuseEnvmapFilterRP::execute(RenderDatabase& database) {

		SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

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
        glUseProgram(shader);
        glBindVertexArray(database.cubeVAO);
        glViewport(0, 0, 512, 512);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, database.environment.cubemap);
        glUniformMatrix4fv(projectionLoc, 1, GL_TRUE, (const GLfloat*) projection.elem);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        for (int i = 0; i < 6; i++) {
            glUniformMatrix4fv(viewLoc, 1, GL_TRUE, (const GLfloat*) captureViews[i].elem);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                    GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                    database.environment.diffuseMap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(0);

		SOUL_PROFILE_RANGE_POP();
    }

    void DiffuseEnvmapFilterRP::shutdown(RenderDatabase& database) {
        glDeleteProgram(shader);
    }
}