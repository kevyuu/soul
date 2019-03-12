#include "core/debug.h"

#include "render/data.h"
#include "render/intern/asset.h"
#include "render/intern/glext.h"

namespace Soul {
	namespace Render {
		void Render::VoxelMipmapGenRP::init(Database& db) {
			program = GLExt::ProgramCreate(RenderAsset::ShaderFile::voxel_mipmap_gen);
		}

		void VoxelMipmapGenRP::execute(Database& db) {
			SOUL_PROFILE_RANGE_PUSH(__FUNCTION__);

			glUseProgram(program);

			int mipLevel = (int)log2f(db.voxelGIConfig.resolution + 1);
			int voxelDstReso = db.voxelGIConfig.resolution;
			for (int i = 0; i < mipLevel - 1; i++) {
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
				glBindImageTexture(0, db.voxelGIBuffer.lightVoxelTex, i, false, 0, GL_READ_ONLY, GL_RGBA16F);
				glBindImageTexture(1, db.voxelGIBuffer.lightVoxelTex, i + 1, false, 0, GL_WRITE_ONLY, GL_RGBA16F);
				voxelDstReso /= 2;
				glDispatchCompute(voxelDstReso, voxelDstReso, voxelDstReso);
			}

			SOUL_PROFILE_RANGE_POP();
		}

		void VoxelMipmapGenRP::shutdown(Database& db) {
			glDeleteProgram(program);
		}
	}
}