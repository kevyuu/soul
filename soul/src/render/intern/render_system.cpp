#include "render/system.h"
#include "render/intern/glext.h"

#include "core/debug.h"
#include "core/math.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <stddef.h>

namespace Soul { namespace Render {

    void System::init(const System::Config &config) {

        Database& db = _db;
		db.frameIdx = 0;

        db.targetWidthPx = config.targetWidthPx;
        db.targetHeightPx = config.targetHeightPx;

        db.materialBuffer.reserve(config.materialPoolSize);
		
		db.meshIndexes.reserve(config.meshPoolSize);
		db.meshRIDs.reserve(config.meshPoolSize);
        db.meshBuffer.reserve(config.meshPoolSize);

		db.dirLightIndexes.reserve(MAX_DIR_LIGHT);
		db.dirLightCount = 0;

		db.wireframeMeshes.reserve(100);

        db.renderPassList.reserve(8);

        // setup scene ubo
        glGenBuffers(1, &db.cameraDataUBOHandle);
        glBindBuffer(GL_UNIFORM_BUFFER, db.cameraDataUBOHandle);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraDataUBO), NULL, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, Constant::CAMERA_DATA_BINDING_POINT, db.cameraDataUBOHandle);

        // setup light ubo
        glGenBuffers(1, &db.lightDataUBOHandle);
        glBindBuffer(GL_UNIFORM_BUFFER, db.lightDataUBOHandle);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(LightDataUBO), NULL, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, Constant::LIGHT_DATA_BINDING_POINT, db.lightDataUBOHandle);
        
		// setup voxel gi ubo
		glGenBuffers(1, &db.voxelGIDataUBOHandle);
		glBindBuffer(GL_UNIFORM_BUFFER, db.voxelGIDataUBOHandle);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(VoxelGIDataUBO), NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_UNIFORM_BUFFER, Constant::VOXEL_GI_DATA_BINDING_POINT, db.voxelGIDataUBOHandle);

		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		shadowAtlasUpdateConfig(config.shadowAtlasConfig);
		voxelGIUpdateConfig(config.voxelGIConfig);

		_flushUBO();

        panoramaToCubemapRP.init(_db);
        diffuseEnvmapFilterRP.init(_db);
        specularEnvmapFilterRP.init(_db);
        brdfMapRP.init(_db);
		voxelizeRP.init(_db);

		_effectBufferInit();
        _gBufferInit();
        _lightBufferInit();
        _utilVAOInit();
        _brdfMapInit();
		_velocityBufferInit();

        db.renderPassList.add(new ShadowMapRP());
        db.renderPassList.add(new GBufferGenRP());
        db.renderPassList.add(new GaussianBlurRP(_db.gBuffer.frameBuffer, GL_COLOR_ATTACHMENT3));
		db.renderPassList.add(new SSRTraceRP());
		db.renderPassList.add(new VoxelLightInjectRP());
		db.renderPassList.add(new VoxelMipmapGenRP());
		db.renderPassList.add(new SSRResolveRP());
		db.renderPassList.add(new GlowExtractRP());
		db.renderPassList.add(new GaussianBlurRP(_db.effectBuffer.lightMipChain[0].mipmaps[0].frameBuffer, GL_COLOR_ATTACHMENT0));
		db.renderPassList.add(new GlowBlendRP());
		db.renderPassList.add(new SkyboxRP());
		db.renderPassList.add(new WireframeRP());

        for (int i = 0; i < db.renderPassList.size(); i++) {
            db.renderPassList.get(i)->init(db);
        }

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

    }

	void System::shaderReload() {
		for (int i = 0; i < _db.renderPassList.size(); i++) {
			_db.renderPassList[i]->init(_db);
		}
	}

	

    void System::_gBufferInit() {

		_gBufferCleanup();

        Database& db = _db;
        GBuffer& gBuffer = _db.gBuffer;

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

	void System::_gBufferCleanup() {
		Database& db = _db;

		GLExt::FramebufferDelete( &db.gBuffer.frameBuffer);
		GLExt::TextureDelete(&db.gBuffer.depthBuffer);
		GLExt::TextureDelete(&db.gBuffer.renderBuffer1);
		GLExt::TextureDelete(&db.gBuffer.renderBuffer2);
		GLExt::TextureDelete(&db.gBuffer.renderBuffer3);
		GLExt::TextureDelete(&db.gBuffer.renderBuffer4);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "GBuffer cleanup error");
	}

    void System::_effectBufferInit() {
		
		_effectBufferCleanup();

        Database& db = _db;
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

		// Initialize post process buffer
		glGenFramebuffers(1, &(effectBuffer.postProcessBuffer.frameBuffer));
		glBindFramebuffer(GL_FRAMEBUFFER, effectBuffer.postProcessBuffer.frameBuffer);

		glGenTextures(1, &(effectBuffer.postProcessBuffer.colorBuffer));
		glBindTexture(GL_TEXTURE_2D, effectBuffer.postProcessBuffer.colorBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, targetWidth, targetHeight, 0, GL_RGB, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, effectBuffer.postProcessBuffer.colorBuffer, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, effectBuffer.depthBuffer, 0);
		
		// Initialize ssr trace buffer
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

		// Initialize resolve buffer
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
        	int w = _db.targetWidthPx;
        	int h = _db.targetHeightPx;

        	if (i == 1) {
        	    w >>= 1;
        	    h >>= 1;
        	}

        	int level = (int) (fmin(log(w + 1) , log(h + 1)) / log(2));

        	_db.effectBuffer.lightMipChain[i].numLevel = level;
        	_db.effectBuffer.lightMipChain[i].mipmaps.reserve(level);

        	glGenTextures(1, &_db.effectBuffer.lightMipChain[i].colorBuffer);
        	glBindTexture(GL_TEXTURE_2D, _db.effectBuffer.lightMipChain[i].colorBuffer);

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
                                       _db.effectBuffer.lightMipChain[i].colorBuffer,
                                       j);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, effectBuffer.depthBuffer, 0);


                status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                std::cout<<"Status : "<<status<<std::endl;
                std::cout<<"Error : "<<GL_FRAMEBUFFER_COMPLETE<<" "<<GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT<<std::endl;

                _db.effectBuffer.lightMipChain[i].mipmaps.add(mipmap);
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

	void System::_effectBufferCleanup() {
		Database& db = _db;
		EffectBuffer& effectBuffer = db.effectBuffer;

		GLExt::TextureDelete(&effectBuffer.postProcessBuffer.colorBuffer);
		GLExt::FramebufferDelete(&effectBuffer.postProcessBuffer.frameBuffer);

		GLExt::TextureDelete(&effectBuffer.ssrTraceBuffer.traceBuffer);
		GLExt::FramebufferDelete( &effectBuffer.ssrTraceBuffer.frameBuffer);
		
		GLExt::TextureDelete(&effectBuffer.ssrResolveBuffer.resolveBuffer);
		GLExt::FramebufferDelete( &effectBuffer.ssrResolveBuffer.frameBuffer);

		GLExt::TextureDelete(&effectBuffer.depthBuffer);

		for (int i = 0; i < 2; i++) {
			EffectBuffer::MipChain& mipChain = db.effectBuffer.lightMipChain[i];

			GLExt::TextureDelete(&mipChain.colorBuffer);

			for (int j = 0; j < mipChain.mipmaps.size(); j++) {
				GLExt::FramebufferDelete( &mipChain.mipmaps.get(j).frameBuffer);
			}
		}

		db.effectBuffer.lightMipChain[0].mipmaps.cleanup();
		db.effectBuffer.lightMipChain[1].mipmaps.cleanup();

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "Effect buffer cleanup error");
	}

    void System::_lightBufferInit() {
		_lightBufferCleanup();

        Database& db = _db;
        LightBuffer& lightBuffer = db.lightBuffer;

        GLsizei targetWidth = db.targetWidthPx;
        GLsizei targetHeight = db.targetHeightPx;

        glGenFramebuffers(1, &(lightBuffer.frameBuffer));
        glBindFramebuffer(GL_FRAMEBUFFER, lightBuffer.frameBuffer);

        glGenTextures(1, &(lightBuffer.colorBuffer));
        glBindTexture(GL_TEXTURE_2D, lightBuffer.colorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, targetWidth, targetHeight, 0, GL_RGB, GL_FLOAT, 0);
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

	void System::_lightBufferCleanup() {
		Database& db = _db;
		LightBuffer& lightBuffer = db.lightBuffer;

		GLExt::FramebufferDelete( &lightBuffer.frameBuffer);
		GLExt::TextureDelete(&lightBuffer.colorBuffer);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "Light buffer cleanup error");
	}

    void System::_brdfMapInit() {
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
        _db.environment.brdfMap = brdfMap;
        brdfMapRP.execute(_db);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");
    }

	void System::_brdfMapCleanup() {

		Database& db = _db;
		GLExt::TextureDelete(&db.environment.brdfMap);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");
	}

	void System::voxelGIVoxelize() {
		voxelizeRP.execute(_db);
	}

	void System::voxelGIUpdateConfig(const VoxelGIConfig& config) {
		_db.voxelGIConfig = config;
		_flushVoxelGIUBO();
		_voxelGIBufferInit();
	}

	void System::_voxelGIBufferInit() {

		_voxelGIBufferCleanup();

		int reso = _db.voxelGIConfig.resolution;

		GLuint voxelAlbedoTex;
		glGenTextures(1, &voxelAlbedoTex);
		glBindTexture(GL_TEXTURE_3D, voxelAlbedoTex);
		glTexStorage3D(GL_TEXTURE_3D, (int)1, GL_RGBA8, reso, reso, reso);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		_db.voxelGIBuffer.gVoxelAlbedoTex = voxelAlbedoTex;

		GLuint voxelNormalTex;
		glGenTextures(1, &voxelNormalTex);
		glBindTexture(GL_TEXTURE_3D, voxelNormalTex);
		glTexStorage3D(GL_TEXTURE_3D, (int)1, GL_RGBA8, reso, reso, reso);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		_db.voxelGIBuffer.gVoxelNormalTex = voxelNormalTex;

		GLuint voxelEmissiveTex;
		glGenTextures(1, &voxelEmissiveTex);
		glBindTexture(GL_TEXTURE_3D, voxelEmissiveTex);
		glTexStorage3D(GL_TEXTURE_3D, (int)1, GL_RGBA8, reso, reso, reso);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		_db.voxelGIBuffer.gVoxelEmissiveTex = voxelEmissiveTex;

		GLuint lightVoxelTex;
		glGenTextures(1, &lightVoxelTex);
		glBindTexture(GL_TEXTURE_3D, lightVoxelTex);
		glTexStorage3D(GL_TEXTURE_3D, (int)log2f(reso), GL_RGBA16F, reso, reso, reso);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		_db.voxelGIBuffer.lightVoxelTex = lightVoxelTex;

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "Voxel GI Buffer initialization error");
	}

	void System::_voxelGIBufferCleanup() {
		
		Database& db = _db;
		VoxelGIBuffer& voxelGIBuffer = db.voxelGIBuffer;

		GLExt::TextureDelete(&voxelGIBuffer.gVoxelAlbedoTex);
		GLExt::TextureDelete(&voxelGIBuffer.gVoxelNormalTex);
		GLExt::TextureDelete(&voxelGIBuffer.lightVoxelTex);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "Voxel GI Buffer cleanup error");

	}

	void System::_velocityBufferInit() {

		_velocityBufferCleanup();

		VelocityBuffer& velocityBuffer = _db.velocityBuffer;
		int targetWidth = _db.targetWidthPx;
		int targetHeight = _db.targetHeightPx;

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

	void System::_velocityBufferCleanup() {
		GLExt::TextureDelete(&_db.velocityBuffer.tex);
		GLExt::FramebufferDelete(&_db.velocityBuffer.frameBuffer);
	}

    void System::_utilVAOInit() {
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

        glGenVertexArrays(1, &_db.cubeVAO);
        glGenBuffers(1, &_db.cubeVBO);
        glBindVertexArray(_db.cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, _db.cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0);

        float quadVertices[] = {
            -1.0f, -1.0f,
            -1.0f, 1.0f,
            1.0f, -1.0f,
            1.0f, 1.0f
        };

        glGenVertexArrays(1, &_db.quadVAO);
        glGenBuffers(1, &_db.quadVBO);
        glBindVertexArray(_db.quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, _db.quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);
        glBindVertexArray(0);

		GLExt::ErrorCheck("_initUtilGeometry");

    }
	
	void System::_utilVAOCleanup() {
		Database& db = _db;
		
		glDeleteBuffers(1, &db.cubeVBO);
		glDeleteVertexArrays(1, &db.cubeVAO);

		glDeleteBuffers(1, &db.quadVBO);
		glDeleteVertexArrays(1, &db.quadVAO);

	}

    void System::shutdown() {

        Database& db = _db;

        for (int i = 0; i < db.renderPassList.size(); i++) {
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

		for (int i = 0; i < db.materialBuffer.size(); i++) {
			GLExt::TextureDelete(&db.materialBuffer[i].albedoMap);
			GLExt::TextureDelete(&db.materialBuffer[i].metallicMap);
			GLExt::TextureDelete(&db.materialBuffer[i].normalMap);
			GLExt::TextureDelete(&db.materialBuffer[i].roughnessMap);
		}

		for (int i = 0; i < db.meshBuffer.size(); i++) {
			glDeleteBuffers(1, &db.meshBuffer[i].eboHandle);
			glDeleteBuffers(1, &db.meshBuffer[i].vboHandle);
			glDeleteVertexArrays(1, &db.meshBuffer[i].vaoHandle);
		}

		db.materialBuffer.cleanup();

		db.meshIndexes.cleanup();
		db.meshRIDs.cleanup();
		db.meshBuffer.cleanup();
        
		db.dirLightIndexes.cleanup();
		db.dirLightCount = 0;
		
		db.pointLights.cleanup();

		db.spotLights.cleanup();
		
		db.wireframeMeshes.cleanup();

		db.renderPassList.cleanup();

        panoramaToCubemapRP.shutdown(_db);
        diffuseEnvmapFilterRP.shutdown(_db);
        specularEnvmapFilterRP.shutdown(_db);
        brdfMapRP.shutdown(_db);
		voxelizeRP.shutdown(_db);

		glDeleteBuffers(1, &db.cameraDataUBOHandle);
		glDeleteBuffers(1, &db.lightDataUBOHandle);

    }

	void System::_shadowAtlasInit() {
		_shadowAtlasCleanup();

		Database& db = _db;

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

	void System::_shadowAtlasCleanup() {

		GLExt::TextureDelete(&_db.shadowAtlas.texHandle);
		GLExt::FramebufferDelete(&_db.shadowAtlas.framebuffer);
		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

		for (int i = 0; i < ShadowAtlas::MAX_LIGHT; i++) {
			_db.shadowAtlas.slots[i] = false;
		}
		
	}

	ShadowKey System::_shadowAtlasGetSlot(int texReso) {
		ShadowKey shadowKey = { -1, -1, -1 };
		int bestSlot = -1;
		int quadrantSize = _db.shadowAtlas.resolution / 2;
		int neededSize = texReso;
		int currentSlotSize = quadrantSize;
		int slotIter = 0;
		for (int i = 0; i < 4; i++) {
			int subdivSize = quadrantSize / _db.shadowAtlas.subdivSqrtCount[i];
			if (subdivSize < neededSize || subdivSize > currentSlotSize) {
				slotIter += _db.shadowAtlas.subdivSqrtCount[i] * _db.shadowAtlas.subdivSqrtCount[i];
				continue;
			}

			for (int j = 0; j < _db.shadowAtlas.subdivSqrtCount[i] * _db.shadowAtlas.subdivSqrtCount[i]; j++) {
				if (_db.shadowAtlas.slots[slotIter] == false) {
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

		_db.shadowAtlas.slots[bestSlot] = true;

		return shadowKey;
	}

	void System::_shadowAtlasFreeSlot(ShadowKey shadowKey) {
		_db.shadowAtlas.slots[shadowKey.slot] = false;
	}

	void System::shadowAtlasUpdateConfig(const ShadowAtlasConfig& config) {
		Database& db = _db;
		db.shadowAtlas.resolution = config.resolution;
		for (int i = 0; i < 4; i++) {
			db.shadowAtlas.subdivSqrtCount[i] = config.subdivSqrtCount[i];
		}
		_shadowAtlasInit();

		for (int i = 0; i < db.dirLightCount; i++) {
			db.dirLights[i].shadowKey = _shadowAtlasGetSlot(db.dirLights[i].resolution);
		}
	}

    MaterialRID System::dirLightCreate(const DirectionalLightSpec &spec) {
		
		uint32 dirLightIndex = _db.dirLightCount;
		
		DirLightRID lightRID = _db.dirLightIndexes.add(dirLightIndex);
		_db.dirLightIndexes.add(dirLightIndex);
		_db.dirLightRIDs[dirLightIndex] = lightRID;

        DirLight& light = _db.dirLights[dirLightIndex];

        light.direction = unit(spec.direction);
        light.color = spec.color;
		light.illuminance = spec.illuminance;
		light.resolution = spec.shadowMapResolution;
        light.shadowKey = _shadowAtlasGetSlot(spec.shadowMapResolution);
		light.bias = spec.bias;
        
		for (int i = 0; i < 3; i++) {
            light.split[i] = spec.split[i];
        }

        _db.dirLightCount++;

        return lightRID;
    }

	void System::dirLightDestroy(DirLightRID lightRID) {
		
    	uint32 dirLightIndex = _db.dirLightIndexes[lightRID];
		_shadowAtlasFreeSlot(_db.dirLights[dirLightIndex].shadowKey);
    	_db.dirLightRIDs[dirLightIndex] = _db.dirLightRIDs[_db.dirLightCount - 1];
		_db.dirLights[dirLightIndex] = _db.dirLights[_db.dirLightCount - 1];
		_db.dirLightIndexes[_db.dirLightRIDs[dirLightIndex]] = dirLightIndex;
		_db.dirLightCount--;

	}

	DirLight* System::dirLightPtr(DirLightRID lightRID) {
		uint32 dirLightIndex = _db.dirLightIndexes[lightRID];
		return _db.dirLights + dirLightIndex;
	}

    void System::dirLightSetDirection(DirLightRID lightRID, Vec3f direction) {
		DirLight* dirLight = dirLightPtr(lightRID);
		dirLight->direction = direction;
    }

	void System::dirLightSetColor(DirLightRID lightRID, Vec3f color) {
		DirLight* dirLight = dirLightPtr(lightRID);
		dirLight->color = color;
	}

	void System::dirLightSetIlluminance(DirLightRID lightRID, float illuminance) {
		DirLight* dirLight = dirLightPtr(lightRID);
		dirLight->illuminance = illuminance;
	}

	void System::dirLightSetShadowMapResolution(DirLightRID lightRID, int32 resolution) {

		SOUL_ASSERT(0, resolution == nextPowerOfTwo(resolution), "");
		DirLight* dirLight = dirLightPtr(lightRID);

		_shadowAtlasFreeSlot(dirLight->shadowKey);
		dirLight->resolution = resolution;
		dirLight->shadowKey = _shadowAtlasGetSlot(resolution);

	}

	void System::dirLightSetCascadeSplit(DirLightRID lightRID, float split1, float split2, float split3)
	{
		DirLight* dirLight = dirLightPtr(lightRID);
		dirLight->split[0] = split1;
		dirLight->split[1] = split2;
		dirLight->split[2] = split3;
	}

	void System::dirLightSetBias(DirLightRID lightRID, float bias) {
		DirLight* dirLight = dirLightPtr(lightRID);
		dirLight->bias = bias;
	}

	PointLightRID System::pointLightCreate(const PointLightSpec & spec)
	{
		PointLightRID rid = _db.pointLights.add({});
		PointLight& pointLight = *pointLightPtr(rid);
		pointLight.position = spec.position;
		pointLight.bias = spec.bias;
		pointLight.color = spec.color;
		pointLight.maxDistance = spec.maxDistance;
		for (int i = 0; i < 6; i++)
		{
			pointLight.shadowKeys[i] = _shadowAtlasGetSlot(spec.shadowMapResolution);
		}
		pointLightSetPower(rid, spec.power);
		return rid;
	}

	void System::pointLightDestroy(PointLightRID lightRID)
	{
		PointLight* light = pointLightPtr(lightRID);
		for (int i = 0; i < 6; i++)
		{
			_shadowAtlasFreeSlot(light->shadowKeys[i]);
		}
		_db.pointLights.remove(lightRID);
	}

	PointLight * System::pointLightPtr(PointLightRID lightRID)
	{
		return &_db.pointLights[lightRID];
	}

	void System::pointLightSetPosition(PointLightRID lightRID, Vec3f position) {
		PointLight* pointLight = pointLightPtr(lightRID);
		pointLight->position = position;
	}

	void System::pointLightSetMaxDistance(PointLightRID lightRID, float maxDistance)
	{
		PointLight* pointLight = pointLightPtr(lightRID);
		pointLight->maxDistance = maxDistance;
	}

	void System::pointLightSetColor(PointLightRID lightRID, Vec3f color)
	{
		PointLight* pointLight = pointLightPtr(lightRID);
		pointLight->color = color;
	}

	void System::pointLightSetPower(PointLightRID lightRID, float power)
	{
		PointLight* pointLight = pointLightPtr(lightRID);
		pointLight->illuminance = power / (4 * PI);
	}

	void System::pointLightSetBias(PointLightRID lightRID, float bias)
	{
		PointLight* pointLight = pointLightPtr(lightRID);
		pointLight->bias = bias;
	}

	SpotLightRID System::spotLightCreate(const SpotLightSpec& spec)
    {
		SpotLightRID rid = _db.spotLights.add({});
		SpotLight& spotLight = *spotLightPtr(rid);
		spotLight.position = spec.position;
		spotLight.direction = spec.direction;
		spotLight.bias = spec.bias;
		spotLight.color = spec.color;
		spotLight.angleOuter = spec.angleOuter;
		spotLight.cosOuter = cosf(spec.angleOuter);
		spotLight.cosInner = cosf(spec.angleInner);
		spotLight.shadowKey = _shadowAtlasGetSlot(spec.shadowMapResolution);
		spotLight.maxDistance = spec.maxDistance;
		spotLightSetPower(rid, spec.power);
		return rid;
    }

	void System::spotLightDestroy(SpotLightRID rid)
    {
		_shadowAtlasFreeSlot(_db.spotLights[rid].shadowKey);
		_db.spotLights.remove(rid);
    }

	SpotLight* System::spotLightPtr(SpotLightRID spotLightRID)
    {
		return _db.spotLights.ptr(spotLightRID);
    }

	void System::spotLightSetPosition(SpotLightRID spotLightRID, Vec3f position)
    {
		SpotLight* spotLight = spotLightPtr(spotLightRID);
		spotLight->position = position;
    }

	void System::spotLightSetDirection(SpotLightRID spotLightRID, Vec3f direction)
    {
		SpotLight* spotLight = spotLightPtr(spotLightRID);
		spotLight->direction = direction;
    }

    void System::spotLightSetAngleInner(SpotLightRID spotLightRID, float angle)
    {
		SpotLight* spotLight = spotLightPtr(spotLightRID);
		spotLight->cosInner = cosf(angle);
    }

    void System::spotLightSetAngleOuter(SpotLightRID spotLightRID, float angle)
    {
		SpotLight* spotLight = spotLightPtr(spotLightRID);
		spotLight->angleOuter = angle;
		spotLight->cosOuter = cosf(angle);
    }

    void System::spotLightSetMaxDistance(SpotLightRID spotLightRID, float maxDistance)
    {
		SpotLight* spotLight = spotLightPtr(spotLightRID);
		spotLight->maxDistance = maxDistance;
    }

    void System::spotLightSetColor(SpotLightRID spotLightRID, Vec3f color)
    {
		SpotLight* spotLight = spotLightPtr(spotLightRID);
		spotLight->color = color;
    }

	void System::spotLightSetPower(SpotLightRID spotLightRID, float power)
	{
		SpotLight* spotLight = spotLightPtr(spotLightRID);
		spotLight->illuminance = power / (2 * PI * (1 - cosf(spotLight->angleOuter / 2.0f)));
	}

    void System::spotLightSetBias(SpotLightRID spotLightRID, float bias)
    {
		SpotLight* spotLight = spotLightPtr(spotLightRID);
		spotLight->bias = bias;
    }

	void System::postProcessUpdateGlow(const GlowConfig & config)
	{
		_db.postProcessConfig.glowConfig = config;
	}

    void System::envSetAmbientColor(Vec3f ambientColor) {
        _db.environment.ambientColor = ambientColor;
    }

    void System::envSetAmbientEnergy(float ambientEnergy) {
        _db.environment.ambientEnergy = ambientEnergy;
    }

	void System::envSetEmissiveScale(float emissiveScale) {
		_db.environment.emissiveScale = emissiveScale;
	}

    MaterialRID System::materialCreate(const MaterialSpec& spec) {
        
		MaterialRID rid = _db.materialBuffer.size();

		_db.materialBuffer.add({
				spec.albedoMap,
				spec.normalMap,
				spec.metallicMap,
				spec.roughnessMap,
				spec.aoMap,
				spec.emissiveMap,

				spec.albedo,
				spec.metallic,
				spec.roughness,
				spec.emissive,
				0
        });

		_materialUpdateFlag(rid, spec);

        return rid;    
	}

	void System::materialSetMetallicTextureChannel(MaterialRID rid, TexChannel textureChannel) {

		SOUL_ASSERT(0, textureChannel >= TexChannel_RED && textureChannel <= TexChannel_ALPHA, "Invalid texture channel");

		uint32 flags = _db.materialBuffer[rid].flags;

		for (int i = 0; i < 4; i++) {
			flags &= ~(MaterialFlag_METALLIC_CHANNEL_RED << i);
		}

		flags |= (MaterialFlag_METALLIC_CHANNEL_RED << textureChannel);

		_db.materialBuffer[rid].flags = flags;

	}

	void System::materialSetRoughnessTextureChannel(MaterialRID rid, TexChannel textureChannel) {

		SOUL_ASSERT(0, textureChannel >= TexChannel_RED && textureChannel <= TexChannel_ALPHA, "Invavlid texture channel");

		uint32 flags = _db.materialBuffer[rid].flags;

		for (int i = 0; i < 4; i++) {
			flags &= ~(MaterialFlag_ROUGHNESS_CHANNEL_RED << i);
		}

		flags |= (MaterialFlag_ROUGHNESS_CHANNEL_RED << textureChannel);

		_db.materialBuffer[rid].flags = flags;

	}

	void System::materialSetAOTextureChannel(MaterialRID rid, TexChannel textureChannel) {
		SOUL_ASSERT(0, textureChannel >= TexChannel_RED && textureChannel <= TexChannel_ALPHA, "Invavlid texture channel");

		uint32 flags = _db.materialBuffer[rid].flags;

		for (int i = 0; i < 4; i++) {
			flags &= ~(MaterialFlag_AO_CHANNEL_RED << i);
		}

		flags |= (MaterialFlag_AO_CHANNEL_RED << textureChannel);

		_db.materialBuffer[rid].flags = flags;
	}

	void System::materialUpdate(MaterialRID rid, const MaterialSpec& spec) {
		_db.materialBuffer[rid] = {
			spec.albedoMap,
			spec.normalMap,
			spec.metallicMap,
			spec.roughnessMap,
			spec.aoMap,
			spec.emissiveMap,

			spec.albedo,
			spec.metallic,
			spec.roughness,
			spec.emissive,

			0
		};

		_materialUpdateFlag(rid, spec);
	}

	void System::_materialUpdateFlag(MaterialRID rid, const MaterialSpec& spec) {

		//TODO: do a version without all this branching
		int flags = 0;
		if (spec.useAlbedoTex) flags |= MaterialFlag_USE_ALBEDO_TEX;
		if (spec.useNormalTex) flags |= MaterialFlag_USE_NORMAL_TEX;
		if (spec.useMetallicTex) flags |= MaterialFlag_USE_METALLIC_TEX;
		if (spec.useRoughnessTex) flags |= MaterialFlag_USE_ROUGHNESS_TEX;
		if (spec.useAOTex) flags |= MaterialFlag_USE_AO_TEX;
		if (spec.useEmissiveTex) flags |= MaterialFlag_USE_EMISSIVE_TEX;

		_db.materialBuffer[rid].flags = flags;

		materialSetMetallicTextureChannel(rid, spec.metallicChannel);
		materialSetRoughnessTextureChannel(rid, spec.roughnessChannel);
		materialSetAOTextureChannel(rid, spec.aoChannel);
	}

	void System::wireframePush(MeshRID meshRID) {
		_db.wireframeMeshes.add(meshPtr(meshRID));
	}

    void System::render(const Camera& camera) {
		_db.frameIdx++;
        
		_db.camera = camera;
		_db.camera.updateExposure();

        _dirLightUpdateShadowMatrix();
		_pointLightUpdateShadowMatrix();
		_spotLightUpdateShadowMatrix();
        _flushUBO();


		voxelGIVoxelize();

        for (int i = 0; i < _db.renderPassList.size(); i++) {
            _db.renderPassList.get(i)->execute(_db);
        }
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
        	std::cout<<"Render::OpenGL error: "<<err<<std::endl;
        }

		_db.prevCamera = camera;
		_db.wireframeMeshes.resize(0);
    }

    MeshRID System::meshCreate(const MeshSpec &spec) {

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

        uint32 meshIndex = _db.meshBuffer.size();
        _db.meshBuffer.add({
                spec.transform,
                VAO,
				VBO,
				EBO,
                spec.vertexCount,
                spec.indexCount,
                spec.material
        });

		MeshRID rid = _db.meshIndexes.add(meshIndex);

		_db.meshRIDs.add(rid);

		if (_db.meshBuffer.size() == 1) {
			SOUL_ASSERT(0, spec.vertexCount > 0, "");
			_db.sceneBound.min = spec.transform * spec.vertexes[0].pos;
			_db.sceneBound.max = spec.transform * spec.vertexes[0].pos;
		}

		for (int i = 0; i < spec.vertexCount; i++) {
			Vertex& vertex = spec.vertexes[i];
			Vec3f worldPos = spec.transform * vertex.pos;
			if (_db.sceneBound.min.x > worldPos.x) _db.sceneBound.min.x = worldPos.x;
			if (_db.sceneBound.min.y > worldPos.y) _db.sceneBound.min.y = worldPos.y;
			if (_db.sceneBound.min.z > worldPos.z) _db.sceneBound.min.z = worldPos.z;
		
			if (_db.sceneBound.max.x < worldPos.x) _db.sceneBound.max.x = worldPos.x;
			if (_db.sceneBound.max.y < worldPos.y) _db.sceneBound.max.y = worldPos.y;
			if (_db.sceneBound.max.z < worldPos.z) _db.sceneBound.max.z = worldPos.z;

		}
        return rid;
    }

	void System::meshDestroy(MeshRID rid) {
		uint32 meshIndex = _db.meshIndexes[rid];

		_db.meshBuffer[meshIndex] = _db.meshBuffer.back();
		_db.meshBuffer.pop();
		
		_db.meshRIDs[meshIndex] = _db.meshRIDs.back();
		_db.meshRIDs.pop();

		_db.meshIndexes[_db.meshRIDs[meshIndex]] = meshIndex;
		
		_db.meshIndexes.remove(rid);
	}

	Mesh* System::meshPtr(MeshRID rid) {
		uint32 meshIndex = _db.meshIndexes[rid];
		return _db.meshBuffer.ptr(meshIndex);
	}

	void System::meshSetTransform(MeshRID rid, const Transform& transform) {
		Mesh* mesh = meshPtr(rid);
		mesh->transform = mat4Transform(transform);
	}

	void System::meshSetTransform(MeshRID rid, const Mat4& transform) {
		Mesh* mesh = meshPtr(rid);
		mesh->transform = transform;
	}

    TextureRID System::textureCreate(const TexSpec &spec, unsigned char *data, int dataChannelCount) {

		SOUL_ASSERT(0, dataChannelCount != 0, "Data channel size must not be zero.", "");

        TextureRID textureHandle;
        glGenTextures(1, &textureHandle);
        glBindTexture(GL_TEXTURE_2D, textureHandle);
        
		static const GLuint NUM_CHANNEL_TO_FORMAT[5] = {
                0,
                GL_RED,
                GL_RG,
                GL_RGB,
                GL_RGBA
        };

        glTexImage2D(GL_TEXTURE_2D, 0, _GL_PIXEL_FORMAT_MAP[spec.pixelFormat], spec.width, spec.height, 0, NUM_CHANNEL_TO_FORMAT[dataChannelCount], GL_UNSIGNED_BYTE, data);
		
		glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _GL_WRAP_MAP[spec.wrapS]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _GL_WRAP_MAP[spec.wrapT]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _GL_FILTER_MAP[spec.filterMin]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _GL_FILTER_MAP[spec.filterMag]); 

		float aniso = 0.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
        
		glBindTexture(GL_TEXTURE_2D, 0);

		SOUL_ASSERT(0, GLExt::IsErrorCheckPass(), "");

        return textureHandle;
    }

    void System::envSetPanorama(const float* data, int width, int height) {

		if (_db.environment.panorama != 0) {
			GLExt::TextureDelete(&_db.environment.panorama);
		}

		GLuint panoramaTex;
		glGenTextures(1, &panoramaTex);
		glBindTexture(GL_TEXTURE_2D, panoramaTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
        if (_db.environment.cubemap != 0) {
            GLExt::TextureDelete(&_db.environment.cubemap);

			SOUL_ASSERT(0, _db.environment.diffuseMap != 0, "Environment diffusemap is not zero when cubemap texture is zero.");
            GLExt::TextureDelete(&_db.environment.diffuseMap);

			SOUL_ASSERT(0, _db.environment.specularMap != 0, "Environment specularmap is not zero when cubemap texture is zero.");
            GLExt::TextureDelete(&_db.environment.specularMap);
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
        _db.environment.cubemap = skybox;
        _db.environment.panorama = panoramaTex;
        panoramaToCubemapRP.execute(_db);

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
        _db.environment.diffuseMap = diffuseMap;
        diffuseEnvmapFilterRP.execute(_db);

        GLuint specularMap;
        glGenTextures(1, &specularMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, specularMap);
        for (int i = 0; i < 6; i++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                         512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        _db.environment.specularMap = specularMap;
        specularEnvmapFilterRP.execute(_db);

    }

    void System::envSetSkybox(const SkyboxSpec& spec) {
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
        _db.environment.cubemap = skybox;
    }

    void System::_dirLightUpdateShadowMatrix() {
        Database& db = _db;
        Camera& camera = db.camera;

        Mat4 viewMat = mat4View(camera.position, camera.position + camera.direction, camera.up);

        Vec3f upVec = Vec3f(0.0f, 1.0f, 0.0f);

        float zNear = camera.perspective.zNear;
        float zFar = camera.perspective.zFar;
        float zDepth = zFar - zNear;
        float fov = camera.perspective.fov;
        float aspectRatio = camera.perspective.aspectRatio;

        for (int i = 0; i < db.dirLightCount; i++) {

            DirLight& light = db.dirLights[i];
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


                light.shadowMatrixes[j] = atlasMatrix * mat4Ortho(-radius, radius, -radius, radius, shadowMapNear, shadowMapFar) *
                                        mat4View(worldFrustumCenter, worldFrustumCenter + light.direction, upVec);

            }
        }
    }

	static Mat4 _getAtlasMatrix(const ShadowAtlas& shadowAtlas, const ShadowKey& shadowKey)
	{
		int quadrant = shadowKey.quadrant;
		int subdiv = shadowKey.subdiv;
		int subdivCount = shadowAtlas.subdivSqrtCount[quadrant] * shadowAtlas.subdivSqrtCount[quadrant];
		int atlasReso = shadowAtlas.resolution;
		int subdivReso = atlasReso / (2 * shadowAtlas.subdivSqrtCount[quadrant]);
		int xSubdiv = subdiv % shadowAtlas.subdivSqrtCount[quadrant];
		int ySubdiv = subdiv / shadowAtlas.subdivSqrtCount[quadrant];
		float subdivUVWidth = (float)(subdivReso * 2) / atlasReso;
		float bottomSubdivUV = -1 + (quadrant / 2) * 1 + ySubdiv * subdivUVWidth;
		float leftSubdivUV = -1 + (quadrant % 2) * 1 + xSubdiv * subdivUVWidth;

		Mat4 atlasMatrix;
		atlasMatrix.elem[0][0] = subdivUVWidth / 2;
		atlasMatrix.elem[0][3] = leftSubdivUV + (subdivUVWidth * 0.5f);
		atlasMatrix.elem[1][1] = subdivUVWidth / 2;
		atlasMatrix.elem[1][3] = bottomSubdivUV + (subdivUVWidth * 0.5f);
		atlasMatrix.elem[2][2] = 1;
		atlasMatrix.elem[3][3] = 1;
		
		return atlasMatrix;

	}

	void System::_pointLightUpdateShadowMatrix()
	{
		for (auto iterator = _db.pointLights.begin(); iterator != _db.pointLights.end(); iterator.next())
		{
			PointLight& light = _db.pointLights.iter(iterator);
			for (int i = 0; i < 6; i++)
			{
				Mat4 atlasMatrix = _getAtlasMatrix(_db.shadowAtlas, light.shadowKeys[i]);
				light.shadowMatrixes[i] = 
					atlasMatrix *
					mat4Perspective(PI / 2, 1, 0.0001f, light.maxDistance) *
					mat4View(light.position, light.position + light.DIRECTION[i], light.DIRECTION[(i + 1) % 6]);
			}
		}
	}

    void System::_spotLightUpdateShadowMatrix()
    {
		for (auto iterator = _db.spotLights.begin(); iterator != _db.spotLights.end(); iterator.next())
		{
			SpotLight& light = _db.spotLights.iter(iterator);

			Mat4 atlasMatrix = _getAtlasMatrix(_db.shadowAtlas, light.shadowKey);
			Vec3f up = cross(light.direction, Vec3f(0.0f, 0.0f, 1.0f)) == Vec3f(0.0f, 0.0f, 0.0f) ? Vec3f(1.0f, 0.0f, 0.0f) : Vec3f(0.0f, 0.0f, 1.0f);

			light.shadowMatrix =
				atlasMatrix *
				mat4Perspective(light.angleOuter * 2, 1, 0.001f, light.maxDistance) *
				mat4View(light.position, light.position + light.direction, up);

		}
    }

    void System::_flushUBO() {

		_flushCameraUBO();
		_flushLightUBO();
		_flushVoxelGIUBO();
    }

	void System::_flushCameraUBO()
    {
		Database &db = _db;

		// flush camera UBO
		db.cameraDataUBO.projection = mat4Transpose(db.camera.projection);
		Mat4 viewMat = mat4View(db.camera.position, db.camera.position +
			db.camera.direction, db.camera.up);
		db.cameraDataUBO.view = mat4Transpose(viewMat);
		Mat4 projectionView = db.camera.projection * viewMat;
		db.cameraDataUBO.projectionView = mat4Transpose(projectionView);
		Mat4 invProjectionView = mat4Inverse(projectionView);
		db.cameraDataUBO.invProjectionView = mat4Transpose(invProjectionView);
		db.cameraDataUBO.position = db.camera.position;
		db.cameraDataUBO.exposure = db.camera.exposure;

		glBindBuffer(GL_UNIFORM_BUFFER, db.cameraDataUBOHandle);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraDataUBO), &db.cameraDataUBO);

    }

	void System::_flushLightUBO()
    {
		Database &db = _db;
		float cameraFar = db.camera.perspective.zFar;
		float cameraNear = db.camera.perspective.zNear;
		float cameraDepth = cameraFar - cameraNear;

		// Flush directional light ubo
		db.lightDataUBO.dirLightCount = db.dirLightCount;
		for (int i = 0; i < db.dirLightCount; i++) {
			DirLightUBO &lightUBO = db.lightDataUBO.dirLights[i];
			DirLight &light = db.dirLights[i];
			lightUBO.color = light.color;
			lightUBO.direction = light.direction;

			for (int j = 0; j < 4; j++) {
				lightUBO.shadowMatrixes[j] = mat4Transpose(light.shadowMatrixes[j]);
			}

			for (int j = 0; j < 3; j++) {
				lightUBO.cascadeDepths[j] = cameraNear + (cameraDepth * light.split[j]);
			}
			lightUBO.cascadeDepths[3] = cameraFar;

			lightUBO.color = db.dirLights[i].color;
			lightUBO.direction = db.dirLights[i].direction;
			lightUBO.bias = db.dirLights[i].bias;
			lightUBO.preExposedIlluminance = db.dirLights[i].illuminance * db.camera.exposure;

		}

		// Flush point light
		db.lightDataUBO.pointLightCount = db.pointLights.size();
		int i = 0;
		for (auto iterator = _db.pointLights.begin(); iterator != _db.pointLights.end(); iterator.next(), i++)
		{
			PointLightUBO &pointLightUBO = db.lightDataUBO.pointLights[i];
			PointLight& pointLight = db.pointLights.iter(iterator);
			pointLightUBO.position = pointLight.position;
			pointLightUBO.bias = pointLight.bias;
			pointLightUBO.color = pointLight.color;
			pointLightUBO.maxDistance = pointLight.maxDistance;
			for (int j = 0; j < 6; j++)
			{
				pointLightUBO.shadowMatrixes[j] = mat4Transpose(pointLight.shadowMatrixes[j]);
			}
			pointLightUBO.preExposedIlluminance = pointLight.illuminance * db.camera.exposure;
		}

		// Flush spot light
		db.lightDataUBO.spotLightCount = db.spotLights.size();
		i = 0;
		for (auto it = db.spotLights.begin(); it != db.spotLights.end(); it.next(), i++)
		{
			SpotLight& spotLight = db.spotLights.iter(it);
			SpotLightUBO& spotLightUBO = db.lightDataUBO.spotLights[i];

			spotLightUBO.position = spotLight.position;
			spotLightUBO.color = spotLight.color;
			spotLightUBO.bias = spotLight.bias;
			spotLightUBO.direction = spotLight.direction;
			spotLightUBO.cosOuter = spotLight.cosOuter;
			spotLightUBO.cosInner = spotLight.cosInner;
			spotLightUBO.maxDistance = spotLight.maxDistance;
			spotLightUBO.shadowMatrix = mat4Transpose(spotLight.shadowMatrix);
			spotLightUBO.preExposedIlluminance = spotLight.illuminance * db.camera.exposure;
		}

		glBindBuffer(GL_UNIFORM_BUFFER, db.lightDataUBOHandle);
		GLExt::ErrorCheck("Bind light ubo");
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightDataUBO), &db.lightDataUBO);
		GLExt::ErrorCheck("Sub light ubo");
    }

	void System::_flushVoxelGIUBO() {
		Database& db = _db;

		db.voxelGIDataUBO.frustumCenter = db.voxelGIConfig.center;
		db.voxelGIDataUBO.resolution = db.voxelGIConfig.resolution;
		db.voxelGIDataUBO.bias = db.voxelGIConfig.bias;
		db.voxelGIDataUBO.frustumHalfSpan = db.voxelGIConfig.halfSpan;
		db.voxelGIDataUBO.diffuseMultiplier = db.voxelGIConfig.diffuseMultiplier;
		db.voxelGIDataUBO.specularMultiplier = db.voxelGIConfig.specularMultiplier;

		glBindBuffer(GL_UNIFORM_BUFFER, db.voxelGIDataUBOHandle);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(VoxelGIDataUBO), &db.voxelGIDataUBO);
	}
}}