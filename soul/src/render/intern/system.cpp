#include "render/system.h"
#include "render/intern/glext.h"
#include "core/math.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <stddef.h>

namespace Soul {

    void RenderSystem::init(const RenderSystem::Config &config) {

        RenderDatabase& db = _database;
		db.frameIdx = 0;

        db.targetWidthPx = config.targetWidthPx;
        db.targetHeightPx = config.targetHeightPx;

        db.materialBuffer.init(config.materialPoolSize);
        db.meshBuffer.init(config.meshPoolSize);
        db.renderPassList.init(8);

        // setup scene ubo
        glGenBuffers(1, &db.cameraDataUBOHandle);
        glBindBuffer(GL_UNIFORM_BUFFER, db.cameraDataUBOHandle);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraDataUBO), NULL, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, RenderConstant::CAMERA_DATA_BINDING_POINT, db.cameraDataUBOHandle);

        // setup light ubo
        glGenBuffers(1, &db.lightDataUBOHandle);
        glBindBuffer(GL_UNIFORM_BUFFER, db.lightDataUBOHandle);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(LightDataUBO), NULL, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, RenderConstant::LIGHT_DATA_BINDING_POINT, db.lightDataUBOHandle);
        
		// setup voxel gi ubo
		glGenBuffers(1, &db.voxelGIDataUBOHandle);
		glBindBuffer(GL_UNIFORM_BUFFER, db.voxelGIDataUBOHandle);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(VoxelGIDataUBO), NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, RenderConstant::VOXEL_GI_DATA_BINDING_POINT, db.voxelGIDataUBOHandle);

		glBindBuffer(GL_UNIFORM_BUFFER, 0);


        db.dirLightCount = 0;

		shadowAtlasUpdateConfig(config.shadowAtlasConfig);
		voxelGIUpdateConfig(config.voxelGIConfig);

		_flushUBO();

        panoramaToCubemapRP.init(_database);
        diffuseEnvmapFilterRP.init(_database);
        specularEnvmapFilterRP.init(_database);
        brdfMapRP.init(_database);
		voxelizeRP.init(_database);

		_effectBufferInit();
        _gBufferInit();
        _lightBufferInit();
        _utilVAOInit();
        _brdfMapInit();
		_velocityBufferInit();

        db.renderPassList.pushBack(new ShadowMapRP());
        db.renderPassList.pushBack(new GBufferGenRP());
        db.renderPassList.pushBack(new GaussianBlurRP());
		db.renderPassList.pushBack(new SSRTraceRP());
		db.renderPassList.pushBack(new VoxelLightInjectRP());
		db.renderPassList.pushBack(new VoxelMipmapGenRP());
		db.renderPassList.pushBack(new SSRResolveRP());
		db.renderPassList.pushBack(new SkyboxRP());
		db.renderPassList.pushBack(new VoxelDebugRP());

        for (int i = 0; i < db.renderPassList.getSize(); i++) {
            db.renderPassList.get(i)->init(db);
        }

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

    }

	void RenderSystem::shaderReload() {
		for (int i = 0; i < _database.renderPassList.getSize(); i++) {
			_database.renderPassList[i]->init(_database);
		}
	}

	void RenderSystem::shadowAtlasUpdateConfig(const ShadowAtlasConfig& config) {
		RenderDatabase& db = _database;
		db.shadowAtlas.resolution = config.resolution;
		for (int i = 0; i < 4; i++) {
			db.shadowAtlas.subdivSqrtCount[i] = config.subdivSqrtCount[i];
		}
		_shadowAtlasInit();

		for (int i = 0; i < db.dirLightCount; i++) {
			db.dirLights[i].shadowKey = _shadowAtlasGetSlot(i, db.dirLights[i].resolution);
		}
	}

	void RenderSystem::_shadowAtlasInit() {
		_shadowAtlasCleanup();

		RenderDatabase& db = _database;

		for (int i = 0; i < ShadowAtlas::MAX_LIGHT; i++) {
			db.shadowAtlas.slots[i] = -1;
		}

		GLuint shadowAtlasTex;
		glGenTextures(1, &shadowAtlasTex);
		glBindTexture(GL_TEXTURE_2D, shadowAtlasTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 
			db.shadowAtlas.resolution, db.shadowAtlas.resolution, 0, 
			GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
		db.shadowAtlas.texHandle = shadowAtlasTex;

		GLuint framebuffer;
		glGenFramebuffers(1, &framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, db.shadowAtlas.texHandle, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		db.shadowAtlas.framebuffer = framebuffer;

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

	}

	void RenderSystem::_shadowAtlasCleanup() {
		GLExt::TextureDelete(&_database.shadowAtlas.texHandle);
		GLExt::FramebufferDelete(&_database.shadowAtlas.framebuffer);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");
	}

    void RenderSystem::_gBufferInit() {

		_gBufferCleanup();

        RenderDatabase& db = _database;
        GBuffer& gBuffer = _database.gBuffer;

        GLsizei targetWidth =  db.targetWidthPx;
        GLsizei targetHeight = db.targetHeightPx;

        std::cout<<targetWidth<<" "<<targetHeight<<std::endl;

        glGenFramebuffers(1, &(gBuffer.frameBuffer));
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.frameBuffer);

        glGenTextures(1, &(gBuffer.depthBuffer));
        glBindTexture(GL_TEXTURE_2D, gBuffer.depthBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, targetWidth, targetHeight, 0,
                     GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gBuffer.depthBuffer, 0);
		GLExt::ErrorCheck("_initGBuffer::depthBuffer");

        glGenTextures(1, &(gBuffer.renderBuffer1));
        glBindTexture(GL_TEXTURE_2D, gBuffer.renderBuffer1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, targetWidth, targetHeight, 0,
                    GL_RGBA, GL_HALF_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBuffer.renderBuffer1, 0);
		GLExt::ErrorCheck("_initGBuffer::renderBuffer1");

        glGenTextures(1, &(gBuffer.renderBuffer2));
        glBindTexture(GL_TEXTURE_2D, gBuffer.renderBuffer2);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, targetWidth, targetHeight, 0,
                     GL_RGBA, GL_HALF_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gBuffer.renderBuffer2, 0);
		GLExt::ErrorCheck("_initGBuffer::renderBuffer2");

        glGenTextures(1, &(gBuffer.renderBuffer3));
        glBindTexture(GL_TEXTURE_2D, gBuffer.renderBuffer3);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, targetWidth, targetHeight, 0,
                     GL_RGBA, GL_HALF_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gBuffer.renderBuffer3, 0);
		GLExt::ErrorCheck("_initGBuffer::renderBuffer3");

		glGenTextures(1, &(gBuffer.renderBuffer4));
		glBindTexture(GL_TEXTURE_2D, gBuffer.renderBuffer4);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, targetWidth, targetHeight, 0,
			GL_RGBA, GL_HALF_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gBuffer.renderBuffer4, 0);
		GLExt::ErrorCheck("_initGBuffer::renderBuffer4");

		unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };

		glDrawBuffers(4, attachments);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        std::cout<<"Status : "<<status<<std::endl;
        std::cout<<"Error : "<<GL_FRAMEBUFFER_COMPLETE<<" "<<GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT<<std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

    }

	void RenderSystem::_gBufferCleanup() {
		RenderDatabase& db = _database;

		GLExt::FramebufferDelete( &db.gBuffer.frameBuffer);
		GLExt::TextureDelete(&db.gBuffer.depthBuffer);
		GLExt::TextureDelete(&db.gBuffer.renderBuffer1);
		GLExt::TextureDelete(&db.gBuffer.renderBuffer2);
		GLExt::TextureDelete(&db.gBuffer.renderBuffer3);
		GLExt::TextureDelete(&db.gBuffer.renderBuffer4);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "GBuffer cleanup error");
	}

    void RenderSystem::_effectBufferInit() {
		
		_effectBufferCleanup();

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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16, targetWidth, targetHeight, 0, GL_RG, GL_UNSIGNED_BYTE, 0);
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

                _database.effectBuffer.lightMipChain[i].mipmaps.pushBack(mipmap);
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

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");
    }

	void RenderSystem::_effectBufferCleanup() {
		RenderDatabase& db = _database;
		EffectBuffer& effectBuffer = db.effectBuffer;

		GLExt::TextureDelete(&effectBuffer.ssrTraceBuffer.traceBuffer);
		GLExt::FramebufferDelete( &effectBuffer.ssrTraceBuffer.frameBuffer);
		
		GLExt::TextureDelete(&effectBuffer.ssrResolveBuffer.resolveBuffer);
		GLExt::FramebufferDelete( &effectBuffer.ssrResolveBuffer.frameBuffer);

		GLExt::TextureDelete(&effectBuffer.depthBuffer);

		for (int i = 0; i < 2; i++) {
			EffectBuffer::MipChain& mipChain = db.effectBuffer.lightMipChain[i];

			GLExt::TextureDelete(&mipChain.colorBuffer);

			for (int j = 0; j < mipChain.mipmaps.getSize(); j++) {
				GLExt::FramebufferDelete( &mipChain.mipmaps.get(j).frameBuffer);
			}
		}

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "Effect buffer cleanup error");
	}

    void RenderSystem::_lightBufferInit() {
		_lightBufferCleanup();

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

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");
    }

	void RenderSystem::_lightBufferCleanup() {
		RenderDatabase& db = _database;
		LightBuffer& lightBuffer = db.lightBuffer;

		GLExt::FramebufferDelete( &lightBuffer.frameBuffer);
		GLExt::TextureDelete(&lightBuffer.colorBuffer);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "Light buffer cleanup error");
	}

    void RenderSystem::_brdfMapInit() {
		_brdfMapCleanup();

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

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");
    }

	void RenderSystem::_brdfMapCleanup() {

		RenderDatabase& db = _database;
		GLExt::TextureDelete(&db.environment.brdfMap);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");
	}

	void RenderSystem::voxelGIVoxelize() {
		voxelizeRP.execute(_database);
	}

	void RenderSystem::voxelGIUpdateConfig(const VoxelGIConfig& config) {
		_database.voxelGIConfig = config;
		_flushVoxelGIUBO();
		_voxelGIBufferInit();
	}

	void RenderSystem::_voxelGIBufferInit() {

		_voxelGIBufferCleanup();

		int reso = _database.voxelGIConfig.resolution;

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

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "Voxel GI Buffer initialization error");
	}

	void RenderSystem::_voxelGIBufferCleanup() {
		
		RenderDatabase& db = _database;
		VoxelGIBuffer& voxelGIBuffer = db.voxelGIBuffer;

		GLExt::TextureDelete(&voxelGIBuffer.gVoxelAlbedoTex);
		GLExt::TextureDelete(&voxelGIBuffer.gVoxelNormalTex);
		GLExt::TextureDelete(&voxelGIBuffer.lightVoxelTex);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "Voxel GI Buffer cleanup error");

	}

	void RenderSystem::_velocityBufferInit() {

		_velocityBufferCleanup();

		VelocityBuffer& velocityBuffer = _database.velocityBuffer;
		int targetWidth = _database.targetWidthPx;
		int targetHeight = _database.targetHeightPx;

		glGenFramebuffers(1, &(velocityBuffer.frameBuffer));
		glBindFramebuffer(GL_FRAMEBUFFER, velocityBuffer.frameBuffer);

		glGenTextures(1, &(velocityBuffer.tex));
		glBindTexture(GL_TEXTURE_2D, velocityBuffer.tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, targetWidth, targetHeight, 0, GL_RG, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, velocityBuffer.tex, 0);

	}

	void RenderSystem::_velocityBufferCleanup() {
		GLExt::TextureDelete(&_database.velocityBuffer.tex);
		GLExt::FramebufferDelete(&_database.velocityBuffer.frameBuffer);
	}

    void RenderSystem::_utilVAOInit() {
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

		GLExt::ErrorCheck("_initUtilGeometry");

    }
	
	void RenderSystem::_utilVAOCleanup() {
		RenderDatabase& db = _database;
		
		glDeleteBuffers(1, &db.cubeVBO);
		glDeleteVertexArrays(1, &db.cubeVAO);

		glDeleteBuffers(1, &db.quadVBO);
		glDeleteVertexArrays(1, &db.quadVAO);

	}

    void RenderSystem::shutdown() {

        RenderDatabase& db = _database;

        for (int i = 0; i < db.renderPassList.getSize(); i++) {
            db.renderPassList.get(i)->shutdown(db);
            free(db.renderPassList.get(i));
        }

		_shadowAtlasCleanup();
		_utilVAOCleanup();
		_brdfMapCleanup();
		_gBufferCleanup();
		_effectBufferCleanup();
		_lightBufferCleanup();
		_voxelGIBufferCleanup();

		for (int i = 0; i < db.materialBuffer.getSize(); i++) {
			GLExt::TextureDelete(&db.materialBuffer[i].albedoMap);
			GLExt::TextureDelete(&db.materialBuffer[i].metallicMap);
			GLExt::TextureDelete(&db.materialBuffer[i].normalMap);
			GLExt::TextureDelete(&db.materialBuffer[i].roughnessMap);
		}

		for (int i = 0; i < db.meshBuffer.getSize(); i++) {
			glDeleteBuffers(1, &db.meshBuffer[i].eboHandle);
			glDeleteBuffers(1, &db.meshBuffer[i].vboHandle);
			glDeleteVertexArrays(1, &db.meshBuffer[i].vaoHandle);
		}

		db.materialBuffer.cleanup();
		db.meshBuffer.cleanup();
        db.renderPassList.cleanup();

        panoramaToCubemapRP.shutdown(_database);
        diffuseEnvmapFilterRP.shutdown(_database);
        specularEnvmapFilterRP.shutdown(_database);
        brdfMapRP.shutdown(_database);
		voxelizeRP.shutdown(_database);

		glDeleteBuffers(1, &db.cameraDataUBOHandle);
		glDeleteBuffers(1, &db.lightDataUBOHandle);

    }

    ShadowKey RenderSystem::_shadowAtlasGetSlot(RenderRID lightID, int texReso) {
        ShadowKey shadowKey = { -1, -1, -1 };
        int bestSlot = -1;
        int quadrantSize = _database.shadowAtlas.resolution / 2;
        int neededSize = texReso;
		int currentSlotSize = quadrantSize;
        int slotIter = 0;
        for (int i = 0; i < 4; i++) {
            int subdivSize = quadrantSize / _database.shadowAtlas.subdivSqrtCount[i];
            if (subdivSize < neededSize || subdivSize > currentSlotSize) {
                slotIter += _database.shadowAtlas.subdivSqrtCount[i] * _database.shadowAtlas.subdivSqrtCount[i];
                continue;
            }

            for (int j = 0; j < _database.shadowAtlas.subdivSqrtCount[i] * _database.shadowAtlas.subdivSqrtCount[i]; j++) {
                if (_database.shadowAtlas.slots[slotIter] == -1 ) {
                    shadowKey.quadrant = i;
                    shadowKey.subdiv = j;
					shadowKey.slot = slotIter;
					currentSlotSize = subdivSize;
                    bestSlot = slotIter;
                }
                slotIter++;
            }
        }

        if (bestSlot == -1) return shadowKey;

        _database.shadowAtlas.slots[bestSlot] = lightID;

        return shadowKey;
    }

	void RenderSystem::_shadowAtlasFreeSlot(ShadowKey shadowKey) {
		_database.shadowAtlas.slots[shadowKey.slot] = -1;
	}

    RenderRID RenderSystem::dirLightCreate(const DirectionalLightSpec &spec) {
        RenderRID lightRID = _database.dirLightCount;
        DirectionalLight& light = _database.dirLights[_database.dirLightCount];
        light.direction = unit(spec.direction);
        light.color = spec.color;
		light.resolution = spec.shadowMapResolution;
        light.shadowKey = _shadowAtlasGetSlot(lightRID, spec.shadowMapResolution);
		light.bias = spec.bias;
        for (int i = 0; i < 3; i++) {
            light.split[i] = spec.split[i];
        }
		

        _database.dirLightCount++;

        return lightRID;
    }

    void RenderSystem::dirLightSetDirection(RenderRID lightRID, Vec3f direction) {
        _database.dirLights[lightRID].direction = direction;
    }

	void RenderSystem::dirLightSetColor(RenderRID lightRID, Vec3f color) {
		_database.dirLights[lightRID].color = color;
	}

	void RenderSystem::dirLightSetShadowMapResolution(RenderRID lightRID, int32 resolution) {

		SOUL_ASSERT(0, resolution == nextPowerOfTwo(resolution), "");
		DirectionalLight& dirLight = _database.dirLights[lightRID];

		_shadowAtlasFreeSlot(dirLight.shadowKey);
		dirLight.resolution = resolution;
		dirLight.shadowKey = _shadowAtlasGetSlot(lightRID, resolution);

	}

	void RenderSystem::dirLightSetCascadeSplit(RenderRID lightRID, float split1, float split2, float split3)
	{
		DirectionalLight& dirLight = _database.dirLights[lightRID];
		dirLight.split[0] = split1;
		dirLight.split[1] = split2;
		dirLight.split[2] = split3;
	}

	void RenderSystem::dirLightSetBias(RenderRID lightRID, float bias) {
		DirectionalLight& dirLight = _database.dirLights[lightRID];
		dirLight.bias = bias;
	}

    void RenderSystem::envSetAmbientColor(Vec3f ambientColor) {
        _database.environment.ambientColor = ambientColor;
    }

    void RenderSystem::envSetAmbientEnergy(float ambientEnergy) {
        _database.environment.ambientEnergy = ambientEnergy;
    }

    RenderRID RenderSystem::materialCreate(const MaterialSpec& spec) {
        
		RenderRID rid = _database.materialBuffer.getSize();

		_database.materialBuffer.pushBack({
				spec.albedoMap,
				spec.normalMap,
				spec.metallicMap,
				spec.roughnessMap,
				spec.aoMap,

				spec.albedo,
				spec.metallic,
				spec.roughness,
				0
        });

		_materialUpdateFlag(rid, spec);

        return rid;    
	}

	void RenderSystem::materialSetMetallicTextureChannel(RenderRID rid, TextureChannel textureChannel) {

		SOUL_ASSERT(0, textureChannel >= TextureChannel_RED && textureChannel <= TextureChannel_ALPHA, "Invalid texture channel");

		uint32 flags = _database.materialBuffer[rid].flags;

		for (int i = 0; i < 4; i++) {
			flags &= ~(MaterialFlag_METALLIC_CHANNEL_RED << i);
		}

		flags |= (MaterialFlag_METALLIC_CHANNEL_RED << textureChannel);

		_database.materialBuffer[rid].flags = flags;

	}

	void RenderSystem::materialSetRoughnessTextureChannel(RenderRID rid, TextureChannel textureChannel) {

		SOUL_ASSERT(0, textureChannel >= TextureChannel_RED && textureChannel <= TextureChannel_ALPHA, "Invavlid texture channel");

		uint32 flags = _database.materialBuffer[rid].flags;

		for (int i = 0; i < 4; i++) {
			flags &= ~(MaterialFlag_ROUGHNESS_CHANNEL_RED << i);
		}

		flags |= (MaterialFlag_ROUGHNESS_CHANNEL_RED << textureChannel);

		_database.materialBuffer[rid].flags = flags;

	}

	void RenderSystem::materialSetAOTextureChannel(RenderRID rid, TextureChannel textureChannel) {
		SOUL_ASSERT(0, textureChannel >= TextureChannel_RED && textureChannel <= TextureChannel_ALPHA, "Invavlid texture channel");

		uint32 flags = _database.materialBuffer[rid].flags;

		for (int i = 0; i < 4; i++) {
			flags &= ~(MaterialFlag_AO_CHANNEL_RED << i);
		}

		flags |= (MaterialFlag_AO_CHANNEL_RED << textureChannel);

		_database.materialBuffer[rid].flags = flags;
	}

	void RenderSystem::materialUpdate(RenderRID rid, const MaterialSpec& spec) {
		_database.materialBuffer[rid] = {
			spec.albedoMap,
			spec.normalMap,
			spec.metallicMap,
			spec.roughnessMap,
			spec.aoMap,

			spec.albedo,
			spec.metallic,
			spec.roughness,

			0
		};

		_materialUpdateFlag(rid, spec);
	}

	void RenderSystem::_materialUpdateFlag(RenderRID rid, const MaterialSpec& spec) {

		//TODO: do a version without all this branching
		int flags = 0;
		if (spec.useAlbedoTex) flags |= MaterialFlag_USE_ALBEDO_TEX;
		if (spec.useNormalTex) flags |= MaterialFlag_USE_NORMAL_TEX;
		if (spec.useMetallicTex) flags |= MaterialFlag_USE_METALLIC_TEX;
		if (spec.useRoughnessTex) flags |= MaterialFlag_USE_ROUGHNESS_TEX;
		if (spec.useAOTex) flags |= MaterialFlag_USE_AO_TEX;

		_database.materialBuffer[rid].flags = flags;

		materialSetMetallicTextureChannel(rid, spec.metallicChannel);
		materialSetRoughnessTextureChannel(rid, spec.roughnessChannel);
		materialSetAOTextureChannel(rid, spec.aoChannel);
	}

    void RenderSystem::render(const Camera& camera) {
		_database.frameIdx++;
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

		_database.prevCamera = camera;
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


        RenderRID rid = _database.meshBuffer.getSize();
        _database.meshBuffer.pushBack({
                spec.transform,
                VAO,
				VBO,
				EBO,
                spec.vertexCount,
                spec.indexCount,
                spec.material
        });

		if (_database.meshBuffer.getSize() == 1) {
			SOUL_ASSERT(0, spec.vertexCount > 0, "");
			_database.sceneBound.min = spec.vertexes[0].pos;
			_database.sceneBound.max = spec.vertexes[0].pos;
		}

		for (int i = 0; i < spec.vertexCount; i++) {
			Vertex& vertex = spec.vertexes[i];
			if (_database.sceneBound.min.x > vertex.pos.x) _database.sceneBound.min.x = vertex.pos.x;
			if (_database.sceneBound.min.y > vertex.pos.y) _database.sceneBound.min.y = vertex.pos.y;
			if (_database.sceneBound.min.z > vertex.pos.z) _database.sceneBound.min.z = vertex.pos.z;
		
			if (_database.sceneBound.max.x < vertex.pos.x) _database.sceneBound.max.x = vertex.pos.x;
			if (_database.sceneBound.max.y < vertex.pos.y) _database.sceneBound.max.y = vertex.pos.y;
			if (_database.sceneBound.max.z < vertex.pos.z) _database.sceneBound.max.z = vertex.pos.z;

		}

        return rid;

    }

	void RenderSystem::meshSetTransform(RenderRID rid, Vec3f position, Vec3f scale, Vec4f rotation) {
		
		_database.meshBuffer[rid].transform = mat4Translate(position) * mat4Scale(scale) * mat4Rotate(rotation.xyz(), rotation.w);
	
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

		float aniso = 0.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
        
		glBindTexture(GL_TEXTURE_2D, 0);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

        return textureHandle;
    }

    void RenderSystem::envSetPanorama(RenderRID panoramaTex) {
		
        if (_database.environment.cubemap != 0) {
            GLExt::TextureDelete(&_database.environment.cubemap);
            GLExt::TextureDelete(&_database.environment.diffuseMap);
            GLExt::TextureDelete(&_database.environment.specularMap);
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
        _database.environment.panorama = panoramaTex;
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

    void RenderSystem::envSetSkybox(const SkyboxSpec& spec) {
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

				float cascadeDepth = (splitOffset[j + 1] - splitOffset[j]) * zDepth;
				float cascadeFarDistance = zNear + splitOffset[j + 1] * zDepth;
				float cascadeFarWidth = tan(camera.perspective.fov / 2) * 2 * cascadeFarDistance;
				float cascadeFarHeight = cascadeFarWidth / camera.perspective.aspectRatio;

				float radius = sqrt(cascadeFarWidth * cascadeFarWidth + cascadeDepth * cascadeDepth + cascadeFarHeight * cascadeFarHeight);

                float texelPerUnit = splitReso / (radius * 2.0f);
                Mat4 texelScaleLightRot = mat4Scale(Soul::Vec3f(texelPerUnit, texelPerUnit, texelPerUnit)) * lightRot;

                Vec3f lightTexelFrustumCenter = texelScaleLightRot * worldFrustumCenter;
                lightTexelFrustumCenter.x = (float)floor(lightTexelFrustumCenter.x);
                lightTexelFrustumCenter.y = (float)floor(lightTexelFrustumCenter.y);
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

				Vec3f sceneBoundCorners[8] = {
					db.sceneBound.min,
					Vec3f(db.sceneBound.min.x, db.sceneBound.min.y, db.sceneBound.max.z),
					Vec3f(db.sceneBound.min.x, db.sceneBound.max.y, db.sceneBound.min.z),
					Vec3f(db.sceneBound.min.x, db.sceneBound.max.y, db.sceneBound.max.z),
					Vec3f(db.sceneBound.max.x, db.sceneBound.min.y, db.sceneBound.min.z),
					Vec3f(db.sceneBound.max.x, db.sceneBound.min.y, db.sceneBound.max.z),
					Vec3f(db.sceneBound.max.x, db.sceneBound.max.y, db.sceneBound.min.z),
					db.sceneBound.max
				};

				float shadowMapFar = dot(light.direction,(sceneBoundCorners[0] - worldFrustumCenter));
				float shadowMapNear = shadowMapFar;

				for (int i = 1; i < 8; i++) {
					float cornerDist = dot(light.direction, sceneBoundCorners[i] - worldFrustumCenter);
					if (cornerDist > shadowMapFar) shadowMapFar = cornerDist;
					if (cornerDist < shadowMapNear) shadowMapNear = cornerDist;
				}


                light.shadowMatrix[j] = atlasMatrix * mat4Ortho(-radius, radius, -radius, radius, shadowMapNear, shadowMapFar) *
                                        mat4View(worldFrustumCenter, worldFrustumCenter + light.direction, upVec);

            }
        }
    }

    void RenderSystem::_flushUBO() {

        RenderDatabase &db = _database;

        db.cameraDataUBO.projection = mat4Transpose(db.camera.projection);
        Mat4 viewMat = mat4View(db.camera.position, db.camera.position +
                                                    db.camera.direction, db.camera.up);
        db.cameraDataUBO.view = mat4Transpose(viewMat);
		Mat4 projectionView = db.camera.projection * viewMat;
		db.cameraDataUBO.projectionView = mat4Transpose(projectionView);
		Mat4 invProjectionView = mat4Inverse(projectionView);
		db.cameraDataUBO.invProjectionView = mat4Transpose(invProjectionView);

		db.cameraDataUBO.position = db.camera.position;

        float cameraFar = db.camera.perspective.zFar;
        float cameraNear = db.camera.perspective.zNear;
        float cameraDepth = cameraFar - cameraNear;

        db.lightDataUBO.dirLightCount = db.dirLightCount;
        for (int i = 0; i < db.dirLightCount; i++) {
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
			lightUBO.bias = db.dirLights[i].bias;

        }

        glBindBuffer(GL_UNIFORM_BUFFER, db.cameraDataUBOHandle);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraDataUBO), &db.cameraDataUBO);

        glBindBuffer(GL_UNIFORM_BUFFER, db.lightDataUBOHandle);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightDataUBO), &db.lightDataUBO);

		_flushVoxelGIUBO();
    }

	void RenderSystem::_flushVoxelGIUBO() {
		RenderDatabase& db = _database;

		db.voxelGIDataUBO.frustumCenter = db.voxelGIConfig.center;
		db.voxelGIDataUBO.resolution = db.voxelGIConfig.resolution;
		db.voxelGIDataUBO.bias = db.voxelGIConfig.bias;
		db.voxelGIDataUBO.frustumHalfSpan = db.voxelGIConfig.halfSpan;
		db.voxelGIDataUBO.diffuseMultiplier = db.voxelGIConfig.diffuseMultiplier;
		db.voxelGIDataUBO.specularMultiplier = db.voxelGIConfig.specularMultiplier;

		glBindBuffer(GL_UNIFORM_BUFFER, db.voxelGIDataUBOHandle);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(VoxelGIDataUBO), &db.voxelGIDataUBO);
	}
}