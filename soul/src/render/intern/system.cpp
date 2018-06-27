#include "render/system.h"
#include "render/intern/util.h"
#include "core/math.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <stddef.h>

namespace Soul {

    void RenderSystem::init(const RenderSystem::Config &config) {

        glEnable(GL_MULTISAMPLE);

        RenderDatabase& db = _database;
        db.targetWidthPx = config.targetWidthPx;
        db.targetHeightPx = config.targetHeightPx;

		db.voxelFrustumCenter = config.voxelGIConfig.center;
		db.voxelFrustumHalfSpan = config.voxelGIConfig.halfSpan;
		db.voxelFrustumReso = config.voxelGIConfig.resolution;

        db.materialBuffer.init(config.materialPoolSize);
        db.meshBuffer.init(config.meshPoolSize);
        db.renderPassList.init(8);

        // setup scene ubo
        glGenBuffers(1, &db.sceneDataUBOHandle);
        glBindBuffer(GL_UNIFORM_BUFFER, db.sceneDataUBOHandle);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(SceneDataUBO), NULL, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, RenderConstant::SCENE_DATA_BINDING_POINT, db.sceneDataUBOHandle);

        // setup light ubo
        glGenBuffers(1, &db.lightDataUBOHandle);
        glBindBuffer(GL_UNIFORM_BUFFER, db.lightDataUBOHandle);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(LightDataUBO), NULL, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, RenderConstant::LIGHT_DATA_BINDING_POINT, db.lightDataUBOHandle);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        db.dirLightCount = 0;

        // setup shadow atlas;
        db.shadowAtlas.resolution = TexReso::TR_8192;
        db.shadowAtlas.subdivSqrtCount[0] = 1;
        db.shadowAtlas.subdivSqrtCount[1] = 1;
        db.shadowAtlas.subdivSqrtCount[2] = 2;
        db.shadowAtlas.subdivSqrtCount[3] = 2;

        for (int i = 0; i < ShadowAtlas::MAX_LIGHT; i++) {
            db.shadowAtlas.slots[i] = -1;
        }

        TextureSpec texSpec;
        texSpec.height = db.shadowAtlas.resolution;
        texSpec.width = db.shadowAtlas.resolution;
        texSpec.mipLevel = 0;
        texSpec.pixelFormat = PF_DEPTH_COMPONENT;
        texSpec.minFilter = GL_LINEAR;
        texSpec.magFilter = GL_LINEAR;
        db.shadowAtlas.texHandle = textureCreate(texSpec, nullptr, 0);

        panoramaToCubemapRP.init(_database);
        diffuseEnvmapFilterRP.init(_database);
        specularEnvmapFilterRP.init(_database);
        brdfMapRP.init(_database);

        db.environment.ambientColor = Vec3f(1.0f, 1.0f, 1.0f);
        db.environment.ambientEnergy = 0.05f;

		_initEffectBuffer();
        _initGBuffer();
        _initLightBuffer();
        _initUtilGeometry();
        _initBRDFMap();
		_initVoxelGIBuffer();


        db.renderPassList.push_back(new ShadowMapRP());
        db.renderPassList.push_back(new GBufferGenRP());
        db.renderPassList.push_back(new GaussianBlurRP());

		db.renderPassList.push_back(new SSRTraceRP());
		// db.renderPassList.push_back(new Texture2DDebugRP());
		db.renderPassList.push_back(new VoxelizeRP());
		db.renderPassList.push_back(new VoxelLightInjectRP());
		db.renderPassList.push_back(new VoxelMipmapGenRP());
		db.renderPassList.push_back(new SSRResolveRP());
		db.renderPassList.push_back(new VoxelDebugRP());

        for (int i = 0; i < db.renderPassList.getSize(); i++) {
            db.renderPassList.get(i)->init(db);
        }

		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) {
			std::cout<<"init::OpenGL error: "<<err<<std::endl;
		}

		SOUL_ASSERT(0, RenderUtil::GLIsErrorCheckPass(), "");

    }

    void RenderSystem::_initGBuffer() {
        RenderDatabase& db = _database;
        GBuffer& gBuffer = _database.gBuffer;

        GLsizei targetWidth =  db.targetWidthPx;
        GLsizei targetHeight = db.targetHeightPx;

        std::cout<<targetWidth<<" "<<targetHeight<<std::endl;

        glGenFramebuffers(1, &(gBuffer.frameBuffer));
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.frameBuffer);

        glGenTextures(1, &(gBuffer.depthBuffer));
        glBindTexture(GL_TEXTURE_2D, gBuffer.depthBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, targetWidth, targetHeight, 0,
                     GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, gBuffer.depthBuffer, 0);
		RenderUtil::GLErrorCheck("_initGBuffer::depthBuffer");

        glGenTextures(1, &(gBuffer.renderBuffer1));
        glBindTexture(GL_TEXTURE_2D, gBuffer.renderBuffer1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, targetWidth, targetHeight, 0,
                    GL_RGBA, GL_HALF_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBuffer.renderBuffer1, 0);
		RenderUtil::GLErrorCheck("_initGBuffer::renderBuffer1");

        glGenTextures(1, &(gBuffer.renderBuffer2));
        glBindTexture(GL_TEXTURE_2D, gBuffer.renderBuffer2);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, targetWidth, targetHeight, 0,
                     GL_RGBA, GL_HALF_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gBuffer.renderBuffer2, 0);
		RenderUtil::GLErrorCheck("_initGBuffer::renderBuffer2");

        glGenTextures(1, &(gBuffer.renderBuffer3));
        glBindTexture(GL_TEXTURE_2D, gBuffer.renderBuffer3);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, targetWidth, targetHeight, 0,
                     GL_RGBA, GL_HALF_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gBuffer.renderBuffer3, 0);
		RenderUtil::GLErrorCheck("_initGBuffer::renderBuffer3");

		glGenTextures(1, &(gBuffer.renderBuffer4));
		glBindTexture(GL_TEXTURE_2D, gBuffer.renderBuffer4);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, targetWidth, targetHeight, 0,
			GL_RGBA, GL_HALF_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gBuffer.renderBuffer4, 0);
		RenderUtil::GLErrorCheck("_initGBuffer::renderBuffer4");

		unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };

		glDrawBuffers(4, attachments);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        std::cout<<"Status : "<<status<<std::endl;
        std::cout<<"Error : "<<GL_FRAMEBUFFER_COMPLETE<<" "<<GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT<<std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

		SOUL_ASSERT(0, RenderUtil::GLIsErrorCheckPass(), "");

    }

    void RenderSystem::_initEffectBuffer() {

        RenderDatabase& db = _database;
        EffectBuffer& effectBuffer = db.effectBuffer;

        GLsizei targetWidth = db.targetWidthPx;
        GLsizei targetHeight = db.targetHeightPx;

		glGenTextures(1, &(effectBuffer.depthBuffer));
		glBindTexture(GL_TEXTURE_2D, effectBuffer.depthBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, targetWidth, targetHeight, 0,
			GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		
        glGenFramebuffers(1, &(effectBuffer.ssrTraceBuffer.frameBuffer));
        glBindFramebuffer(GL_FRAMEBUFFER, effectBuffer.ssrTraceBuffer.frameBuffer);

        glGenTextures(1, &(effectBuffer.ssrTraceBuffer.traceBuffer));
        glBindTexture(GL_TEXTURE_2D, effectBuffer.ssrTraceBuffer.traceBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, targetWidth, targetHeight, 0, GL_RG, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, effectBuffer.ssrTraceBuffer.traceBuffer, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, effectBuffer.depthBuffer, 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		std::cout<<"Status : "<<status<<std::endl;
		std::cout<<"Error : "<<GL_FRAMEBUFFER_COMPLETE<<" "<<GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT<<std::endl;

		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) {
			std::cout<<"_initEffectBuffer::OpenGL error: "<<err<<std::endl;
		}

        glGenFramebuffers(1, &(db.effectBuffer.ssrResolveBuffer.frameBuffer));
        glBindFramebuffer(GL_FRAMEBUFFER, db.effectBuffer.ssrResolveBuffer.frameBuffer);

        glGenTextures(1, &(db.effectBuffer.ssrResolveBuffer.resolveBuffer));
        glBindTexture(GL_TEXTURE_2D, db.effectBuffer.ssrResolveBuffer.resolveBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, targetWidth, targetHeight, 0, GL_RGB, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               db.effectBuffer.ssrResolveBuffer.resolveBuffer, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, effectBuffer.depthBuffer, 0);

        status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        std::cout<<"Status : "<<status<<std::endl;
        std::cout<<"Error : "<<GL_FRAMEBUFFER_COMPLETE<<" "<<GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT<<std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cout<<"_initReflectionBuffer::OpenGL error: "<<err<<std::endl;
        }

        for (int i = 0; i < 2; i++) {
        	int w = _database.targetWidthPx;
        	int h = _database.targetHeightPx;

        	if (i == 1) {
        	    w >>= 1;
        	    h >>= 1;
        	}

        	int level = (int) (fmin(log(w + 1) , log(h + 1)) / log(2));

        	_database.effectBuffer.lightMipChain[i].numLevel = level;
        	_database.effectBuffer.lightMipChain[i].mipmaps.init(level);

        	glGenTextures(1, &_database.effectBuffer.lightMipChain[i].colorBuffer);
        	glBindTexture(GL_TEXTURE_2D, _database.effectBuffer.lightMipChain[i].colorBuffer);

        	for (int j = 0; j < level; j++) {
                EffectBuffer::MipChain::Mipmap mipmap;
                mipmap.width = w;
                mipmap.height = h;

                glTexImage2D(GL_TEXTURE_2D, j, GL_RGB, w, h, 0, GL_RGB, GL_FLOAT, 0);

                glGenFramebuffers(1, &mipmap.frameBuffer);
                glBindFramebuffer(GL_FRAMEBUFFER, mipmap.frameBuffer);
                glFramebufferTexture2D(GL_FRAMEBUFFER,
                                       GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_2D,
                                       _database.effectBuffer.lightMipChain[i].colorBuffer,
                                       j);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, effectBuffer.depthBuffer, 0);


                status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                std::cout<<"Status : "<<status<<std::endl;
                std::cout<<"Error : "<<GL_FRAMEBUFFER_COMPLETE<<" "<<GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT<<std::endl;

                _database.effectBuffer.lightMipChain[i].mipmaps.push_back(mipmap);
                w >>= 1;
                h >>= 1;
        	}

        	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, level - 1);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

		SOUL_ASSERT(0, RenderUtil::GLIsErrorCheckPass(), "");
    }

    void RenderSystem::_initLightBuffer() {
        RenderDatabase& db = _database;
        LightBuffer& lightBuffer = db.lightBuffer;

        GLsizei targetWidth = db.targetWidthPx;
        GLsizei targetHeight = db.targetHeightPx;

        glGenFramebuffers(1, &(lightBuffer.frameBuffer));
        glBindFramebuffer(GL_FRAMEBUFFER, lightBuffer.frameBuffer);

        glGenTextures(1, &(lightBuffer.colorBuffer));
        glBindTexture(GL_TEXTURE_2D, lightBuffer.colorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, targetWidth, targetHeight, 0, GL_RGB, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightBuffer.colorBuffer, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, db.effectBuffer.depthBuffer, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

		SOUL_ASSERT(0, RenderUtil::GLIsErrorCheckPass(), "");
    }

    void RenderSystem::_initBRDFMap() {

        GLuint brdfMap;
        glGenTextures(1, &brdfMap);
        glBindTexture(GL_TEXTURE_2D, brdfMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16, 512, 512, 0, GL_RG, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        _database.environment.brdfMap = brdfMap;
        brdfMapRP.execute(_database);

		SOUL_ASSERT(0, RenderUtil::GLIsErrorCheckPass(), "");
    }

	void RenderSystem::_initVoxelGIBuffer() {

		int reso = _database.voxelFrustumReso;

		GLuint voxelAlbedoTex;
		glGenTextures(1, &voxelAlbedoTex);
		glBindTexture(GL_TEXTURE_3D, voxelAlbedoTex);
		glTexStorage3D(GL_TEXTURE_3D, (int)1, GL_RGBA8, reso, reso, reso);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		_database.voxelGIBuffer.gVoxelAlbedoTex = voxelAlbedoTex;

		GLuint voxelNormalTex;
		glGenTextures(1, &voxelNormalTex);
		glBindTexture(GL_TEXTURE_3D, voxelNormalTex);
		glTexStorage3D(GL_TEXTURE_3D, (int)1, GL_RGBA8, reso, reso, reso);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		_database.voxelGIBuffer.gVoxelNormalTex = voxelNormalTex;

		GLuint lightVoxelTex;
		glGenTextures(1, &lightVoxelTex);
		glBindTexture(GL_TEXTURE_3D, lightVoxelTex);
		glTexStorage3D(GL_TEXTURE_3D, (int)log2f(reso), GL_RGBA16F, reso, reso, reso);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		_database.voxelGIBuffer.lightVoxelTex = lightVoxelTex;

		SOUL_ASSERT(0, RenderUtil::GLIsErrorCheckPass(), "Voxel GI Buffer initialization error");
	}

    void RenderSystem::_initUtilGeometry() {
        float cubeVertices[] = {
                // positions
                -1.0f,  1.0f, -1.0f,
                -1.0f, -1.0f, -1.0f,
                1.0f, -1.0f, -1.0f,
                1.0f, -1.0f, -1.0f,
                1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,

                -1.0f, -1.0f,  1.0f,
                -1.0f, -1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f,  1.0f,
                -1.0f, -1.0f,  1.0f,

                1.0f, -1.0f, -1.0f,
                1.0f, -1.0f,  1.0f,
                1.0f,  1.0f,  1.0f,
                1.0f,  1.0f,  1.0f,
                1.0f,  1.0f, -1.0f,
                1.0f, -1.0f, -1.0f,

                -1.0f, -1.0f,  1.0f,
                -1.0f,  1.0f,  1.0f,
                1.0f,  1.0f,  1.0f,
                1.0f,  1.0f,  1.0f,
                1.0f, -1.0f,  1.0f,
                -1.0f, -1.0f,  1.0f,

                -1.0f,  1.0f, -1.0f,
                1.0f,  1.0f, -1.0f,
                1.0f,  1.0f,  1.0f,
                1.0f,  1.0f,  1.0f,
                -1.0f,  1.0f,  1.0f,
                -1.0f,  1.0f, -1.0f,

                -1.0f, -1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f,
                1.0f, -1.0f, -1.0f,
                1.0f, -1.0f, -1.0f,
                -1.0f, -1.0f,  1.0f,
                1.0f, -1.0f,  1.0f
        };

        glGenVertexArrays(1, &_database.cubeVAO);
        glGenBuffers(1, &_database.cubeVBO);
        glBindVertexArray(_database.cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, _database.cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);

        float quadVertices[] = {
            -1.0f, -1.0f,
            -1.0f, 1.0f,
            1.0f, -1.0f,
            1.0f, 1.0f
        };

        glGenVertexArrays(1, &_database.quadVAO);
        glGenBuffers(1, &_database.quadVBO);
        glBindVertexArray(_database.quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, _database.quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);
        glBindVertexArray(0);

		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) {
			std::cout<<"_initUtilGeometry::OpenGL error: "<<err<<std::endl;
		}

    }

    void RenderSystem::shutdown() {
        _database.shutdown();

        RenderDatabase& db = _database;

        for (int i = 0; i < db.renderPassList.getSize(); i++) {
            db.renderPassList.get(i)->shutdown(db);
            free(db.renderPassList.get(i));
        }

        db.renderPassList.shutdown();

        panoramaToCubemapRP.shutdown(_database);
        diffuseEnvmapFilterRP.shutdown(_database);
        specularEnvmapFilterRP.shutdown(_database);
        brdfMapRP.shutdown(_database);
    }

    ShadowKey RenderSystem::_getShadowAtlasSlot(RenderRID lightID, TexReso texReso) {
        ShadowKey shadowKey = { -1, -1 };
        int bestSlot = -1;
        int quadrantSize = _database.shadowAtlas.resolution / 2;
        int neededSize = texReso;
        int slotIter = 0;
        for (int i = 0; i < 4; i++) {
            int subdivSize = quadrantSize / _database.shadowAtlas.subdivSqrtCount[i];
            if (subdivSize < neededSize) {
                slotIter += _database.shadowAtlas.subdivSqrtCount[i] * _database.shadowAtlas.subdivSqrtCount[i];
                continue;
            }
            for (int j = 0; j < _database.shadowAtlas.subdivSqrtCount[i] * _database.shadowAtlas.subdivSqrtCount[i]; j++) {
                if (_database.shadowAtlas.slots[slotIter] == -1) {
                    shadowKey.quadrant = i;
                    shadowKey.subdiv = j;
                    bestSlot = slotIter;
                }
                slotIter++;
            }
        }

        if (bestSlot == -1) return shadowKey;

        _database.shadowAtlas.slots[bestSlot] = lightID;
        return shadowKey;
    }

    RenderRID RenderSystem::createDirectionalLight(const DirectionalLightSpec &spec) {
        RenderRID lightRID = _database.dirLightCount;
        DirectionalLight& light = _database.dirLights[_database.dirLightCount];

        light.direction = unit(spec.direction);
        light.color = spec.color;
        light.shadowKey = _getShadowAtlasSlot(lightRID, spec.shadowMapResolution);
        for (int i = 0; i < 3; i++) {
            light.split[i] = spec.split[i];
        }
        _database.dirLightCount++;

        return lightRID;
    }

    void RenderSystem::setDirLightDirection(RenderRID lightRID, Vec3f direction) {
        _database.dirLights[lightRID].direction = direction;
    }

    void RenderSystem::setAmbientColor(Vec3f ambientColor) {
        _database.environment.ambientColor = ambientColor;
    }

    void RenderSystem::setAmbientEnergy(float ambientEnergy) {
        _database.environment.ambientEnergy = ambientEnergy;
    }

    RenderRID RenderSystem::createMaterial(const MaterialSpec& spec) {
        RenderRID rid = _database.materialBuffer.getSize();
        _database.materialBuffer.push_back({
                spec.albedoMap,
                spec.normalMap,
                spec.metallicMap,
                spec.roughnessMap,
                spec.aoMap,
                spec.shaderID
        });
        return rid;
    }

    void RenderSystem::render(const Camera& camera) {
        _database.camera = camera;
        _updateShadowMatrix();
        _flushUBO();

        for (int i = 0; i < _database.renderPassList.getSize(); i++) {
            _database.renderPassList.get(i)->execute(_database);
        }
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
        	std::cout<<"Render::OpenGL error: "<<err<<std::endl;
        }
    }

    RenderRID RenderSystem::meshCreate(const Soul::MeshSpec &spec) {

        GLuint VAO, VBO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER, spec.vertexCount * sizeof(Vertex), &spec.vertexes[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,  spec.indexCount * sizeof(uint32),
                     &spec.indices[0], GL_STATIC_DRAW);

        // vertex positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        // vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texUV));

        // vertex binormal
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, binormal));

        // vertex tangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

        glBindVertexArray(0);


        RenderRID rid = _database.materialBuffer.getSize();
        _database.meshBuffer.push_back({
                spec.transform,
                VAO,
                spec.vertexCount,
                spec.indexCount,
                spec.material
        });

        return rid;

    }

    RenderRID RenderSystem::textureCreate(const TextureSpec &spec, unsigned char *data, int dataChannelCount) {
        RenderRID textureHandle;
        glGenTextures(1, &textureHandle);
        glBindTexture(GL_TEXTURE_2D, textureHandle);
        static const GLuint format[5] = {
                GL_DEPTH_COMPONENT,
                GL_RED,
                GL_RG,
                GL_RGB,
                GL_RGBA
        };
        glTexImage2D(GL_TEXTURE_2D, 0, s_formatMap[spec.pixelFormat], spec.width, spec.height, 0, format[dataChannelCount], GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, spec.minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, spec.magFilter); 
        glBindTexture(GL_TEXTURE_2D, 0);

		SOUL_ASSERT(0, RenderUtil::GLIsErrorCheckPass(), "");

        return textureHandle;
    }

    void RenderSystem::setPanorama(RenderRID panorama) {
        if (_database.environment.cubemap != 0) {
            glDeleteTextures(1, &_database.environment.cubemap);
            _database.environment.cubemap = 0;
            glDeleteTextures(1, &_database.environment.diffuseMap);
            _database.environment.diffuseMap = 0;
            glDeleteTextures(1, &_database.environment.specularMap);
            _database.environment.specularMap = 0;
        }

        GLuint skybox;
        glGenTextures(1, &skybox);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);
        for (int i = 0; i < 6; i++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                         512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        _database.environment.cubemap = skybox;
        _database.environment.panorama = panorama;
        panoramaToCubemapRP.execute(_database);

        GLuint diffuseMap;
        glGenTextures(1, &diffuseMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, diffuseMap);
        for (int i = 0; i < 6; i++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                         512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        _database.environment.diffuseMap = diffuseMap;
        diffuseEnvmapFilterRP.execute(_database);

        GLuint specularMap;
        glGenTextures(1, &specularMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, specularMap);
        for (int i = 0; i < 6; i++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                         128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        _database.environment.specularMap = specularMap;
        specularEnvmapFilterRP.execute(_database);

    }

    void RenderSystem::setSkybox(const SkyboxSpec& spec) {
        GLuint skybox;
        glGenTextures(1, &skybox);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);
        for (int i = 0; i < 6; i ++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, spec.width, spec.height, 0,
                         GL_RGB, GL_UNSIGNED_BYTE, spec.faces[i]);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        _database.environment.cubemap = skybox;
    }

    void RenderSystem::_updateShadowMatrix() {
        RenderDatabase& db = _database;
        Camera& camera = db.camera;

        Mat4 viewMat = mat4View(camera.position, camera.position + camera.direction, camera.up);

        Vec3f upVec = Vec3f(0.0f, 1.0f, 0.0f);

        float zNear = camera.perspective.zNear;
        float zFar = camera.perspective.zFar;
        float zDepth = zFar - zNear;
        float fov = camera.perspective.fov;
        float aspectRatio = camera.perspective.aspectRatio;

        for (int i = 0; i < db.dirLightCount; i++) {

            DirectionalLight& light = db.dirLights[i];
            Mat4 lightRot = mat4View(Vec3f(0, 0, 0), light.direction, upVec);

            float splitOffset[5] = { 0, light.split[0], light.split[1], light.split[2], 1 };

            int quadrant = light.shadowKey.quadrant;
            int subdiv = light.shadowKey.subdiv;
            int subdivCount = db.shadowAtlas.subdivSqrtCount[quadrant] * db.shadowAtlas.subdivSqrtCount[quadrant];
            int atlasReso = db.shadowAtlas.resolution;
            int subdivReso = atlasReso / (2 * db.shadowAtlas.subdivSqrtCount[quadrant]);
            int splitReso = subdivReso / 2;
            int xSubdiv = subdiv % db.shadowAtlas.subdivSqrtCount[quadrant];
            int ySubdiv = subdiv / db.shadowAtlas.subdivSqrtCount[quadrant];

            float subdivUVWidth = (float)(subdivReso * 2) / atlasReso;
            float splitUVWidth = subdivUVWidth / 2.0f;

            float bottomSubdivUV = -1 + (quadrant / 2) * 1 + ySubdiv * subdivUVWidth;
            float leftSubdivUV = -1 + (quadrant % 2) * 1 + xSubdiv * subdivUVWidth;

            for (int j = 0; j < 4; j++) {

                Vec3f frustumCorners[8] = {
                        Vec3f(-1.0f, -1.0f, -1), Vec3f(1.0f, -1.0f, -1), Vec3f(1.0f, 1.0f, -1), Vec3f(-1.0f, 1.0f, -1),
                        Vec3f(-1.0f, -1.0f, 1), Vec3f(1.0f, -1.0f, 1), Vec3f(1.0f, 1.0f, 1), Vec3f(-1.0f, 1.0f, 1)
                };

                Mat4 projectionMat = mat4Perspective(fov, aspectRatio, zNear + splitOffset[j] * zDepth, zNear + splitOffset[j + 1] * zDepth);
                Mat4 projectionViewMat = projectionMat * viewMat;
                Mat4 invProjectionViewMat = mat4Inverse(projectionViewMat);

                Vec3f worldFrustumCenter = Vec3f(0, 0, 0);

                for (int k = 0; k < 8; k++) {
                    Vec4f frustumCorner = invProjectionViewMat * Vec4f(frustumCorners[k], 1.0f);
                    frustumCorners[k] = frustumCorner.xyz() / frustumCorner.w;
                    worldFrustumCenter += frustumCorners[k];
                }
                worldFrustumCenter *= (1.0f / 8.0f);

                Vec3f min = frustumCorners[0];
                Vec3f max = frustumCorners[0];
                for (int k = 0; k < 8; k++) {
                    if (min.x > frustumCorners[k].x) min.x = frustumCorners[k].x;
                    if (min.y > frustumCorners[k].y) min.y = frustumCorners[k].y;
                    if (min.z > frustumCorners[k].z) min.z = frustumCorners[k].z;

                    if (max.x < frustumCorners[k].x) max.x = frustumCorners[k].x;
                    if (max.y < frustumCorners[k].y) max.y = frustumCorners[k].y;
                    if (max.z < frustumCorners[k].z) max.z = frustumCorners[k].z;
                }

                float radius = length(max - min) / 2.0f;
                float texelPerUnit = splitReso / (radius * 2.0f);
                Mat4 texelScaleLightRot = mat4Scale(texelPerUnit, texelPerUnit, texelPerUnit) * lightRot;

                Vec3f lightTexelFrustumCenter = texelScaleLightRot * worldFrustumCenter;
                lightTexelFrustumCenter.x = (float)floor(lightTexelFrustumCenter.x);
                lightTexelFrustumCenter.y = (float)floor(lightTexelFrustumCenter.y);
                lightTexelFrustumCenter.z = (float)floor(lightTexelFrustumCenter.z);
                worldFrustumCenter = mat4Inverse(texelScaleLightRot) * lightTexelFrustumCenter;

                int xSplit = j % 2;
                int ySplit = j / 2;

                float bottomSplitUV = bottomSubdivUV + (ySplit * splitUVWidth);
                float leftSplitUV = leftSubdivUV + (xSplit * splitUVWidth);

                Mat4 atlasMatrix;
                atlasMatrix.elem[0][0] = splitUVWidth / 2;
                atlasMatrix.elem[0][3] = leftSplitUV + (splitUVWidth * 0.5f);
                atlasMatrix.elem[1][1] = splitUVWidth / 2;
                atlasMatrix.elem[1][3] = bottomSplitUV + (splitUVWidth * 0.5f);
                atlasMatrix.elem[2][2] = 1;
                atlasMatrix.elem[3][3] = 1;

                light.shadowMatrix[j] = atlasMatrix * mat4Ortho(-radius, radius, -radius, radius, -2000, 2000) *
                                        mat4View(worldFrustumCenter, worldFrustumCenter + light.direction, upVec);

            }
        }
    }

    void RenderSystem::_flushUBO() {

        RenderDatabase &db = _database;

        db.sceneDataUBO.projection = mat4Transpose(db.camera.projection);
        Mat4 viewMat = mat4View(db.camera.position, db.camera.position +
                                                    db.camera.direction, db.camera.up);
        db.sceneDataUBO.view = mat4Transpose(viewMat);

        float cameraFar = db.camera.perspective.zFar;
        float cameraNear = db.camera.perspective.zNear;
        float cameraDepth = cameraFar - cameraNear;

        db.lightDataUBO.dirLightCount = db.dirLightCount;
        for (int i = 0; i < 1; i++) {
            DirectionalLightUBO &lightUBO = db.lightDataUBO.dirLights[i];
            DirectionalLight &light = db.dirLights[i];
            lightUBO.color = light.color;
            lightUBO.direction = light.direction;

            for (int j = 0; j < 4; j++) {
                lightUBO.shadowMatrix[j] = mat4Transpose(light.shadowMatrix[j]);
            }

            for (int j = 0; j < 3; j++) {
                lightUBO.cascadeDepths[j] = cameraNear + (cameraDepth * light.split[j]);
            }
            lightUBO.cascadeDepths[3] = cameraFar;

            lightUBO.color = db.dirLights[i].color;
            lightUBO.direction = db.dirLights[i].direction;
            for (int j = 0; j < 4; j++) {
                lightUBO.shadowMatrix[j] = mat4Transpose(light.shadowMatrix[j]);
            }
        }

        glBindBuffer(GL_UNIFORM_BUFFER, db.sceneDataUBOHandle);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SceneDataUBO), &db.sceneDataUBO);

        glBindBuffer(GL_UNIFORM_BUFFER, db.lightDataUBOHandle);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightDataUBO), &db.lightDataUBO);
    }

}