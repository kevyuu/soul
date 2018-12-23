//
// Created by Kevin Yudi Utama on 8/8/18.
//

#include "render/type.h"
#include "render/intern/asset.h"
#include "render/intern/glext.h"
#include "core/math.h"

namespace Soul {

    void SpecularEnvmapFilterRP::init(RenderDatabase& database) {
        shader = GLExt::ProgramCreate(RenderAsset::ShaderFile::specularEnvmapFilter);

        projectionLoc = glGetUniformLocation(shader, "projection");
        viewLoc = glGetUniformLocation(shader, "view");
        roughenssLoc = glGetUniformLocation(shader, "roughness");

        glGenFramebuffers(1, &renderTarget);
        glBindFramebuffer(GL_FRAMEBUFFER, renderTarget);
        glGenRenderbuffers(1, &renderBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void SpecularEnvmapFilterRP::execute(RenderDatabase& database) {
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
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, database.environment.cubemap);
        glUniformMatrix4fv(projectionLoc, 1, GL_TRUE, (const GLfloat*) projection.elem);

        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        int maxMipLevel = 5;
        int mipWidth = 128;
        int mipHeight = 128;
        for (int i = 0; i < maxMipLevel; i++, mipWidth /= 2, mipHeight /= 2) {
            for (int j = 0; j < 6; j++) {

                float roughness = i / (maxMipLevel - 1);
                glUniform1f(roughenssLoc, roughness);
                glUniformMatrix4fv(viewLoc, 1, GL_TRUE, (const GLfloat*) captureViews[j].elem);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_CUBE_MAP_POSITIVE_X + j,
                                       database.environment.specularMap, i);
                glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
                glViewport(0, 0, mipWidth, mipHeight);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glDrawArrays(GL_TRIANGLES, 0, 36);

            }
        }
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(0);
    }

    void SpecularEnvmapFilterRP::shutdown(RenderDatabase &database) {
        glDeleteFramebuffers(1, &renderTarget);
        glDeleteRenderbuffers(1, &renderBuffer);
        glDeleteProgram(shader);
    }

}