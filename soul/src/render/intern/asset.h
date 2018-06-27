//
// Created by Kevin Yudi Utama on 7/7/18.
//

#ifndef SOUL_RENDER_INTERN_ASSET_H
#define SOUL_RENDER_INTERN_ASSET_H

#define SHADER_DIR "C:/Dev/soul/soul/src/shaders/"
#define SHADER_FILE_PATH(filename) SHADER_DIR filename

#include "render/type.h"
#include <cstdio>
#include <cstdlib>

namespace Soul {
    namespace RenderAsset {

        namespace ShaderFile {

            constexpr const char* shadowMap = SHADER_FILE_PATH("shadow_map.glsl");
            constexpr const char* pbr = SHADER_FILE_PATH("pbr.glsl");
            constexpr const char* panoramaToCubemap = SHADER_FILE_PATH("panorama_to_cubemap.glsl");
            constexpr const char* diffuseEnvmapFilter = SHADER_FILE_PATH("diffuse_envmap_filter.glsl");
            constexpr const char* specularEnvmapFilter = SHADER_FILE_PATH("specular_envmap_filter.glsl");
            constexpr const char* brdfMap = SHADER_FILE_PATH("brdf_map.glsl");
            constexpr const char* skybox = SHADER_FILE_PATH("skybox.glsl");
            constexpr const char* predepth = SHADER_FILE_PATH("predepth.glsl");
            constexpr const char* gbufferGen = SHADER_FILE_PATH("gbuffer_gen.glsl");
            constexpr const char* lighting = SHADER_FILE_PATH("lighting.glsl");
            constexpr const char* ssrTrace = SHADER_FILE_PATH("ssr_trace.glsl");
            constexpr const char* ssrResolve = SHADER_FILE_PATH("ssr_resolve.glsl");
            constexpr const char* gaussianBlurHorizontal = SHADER_FILE_PATH("gaussian_blur_horizontal.glsl");
            constexpr const char* gaussianBlurVertical = SHADER_FILE_PATH("gaussian_blur_vertical.glsl");
			constexpr const char* texture2dDebug = SHADER_FILE_PATH("texture2d_debug.glsl");
			constexpr const char* voxelize = SHADER_FILE_PATH("voxelize.glsl");
			constexpr const char* voxel_debug = SHADER_FILE_PATH("voxel_debug.glsl");
			constexpr const char* voxel_light_inject = SHADER_FILE_PATH("voxel_light_inject.glsl");
			constexpr const char* voxel_mipmap_gen = SHADER_FILE_PATH("voxel_mipmap_gen.glsl");
        };

    }
}


#endif