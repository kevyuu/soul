#pragma once

#include "panels.h"
#include "menu_bar.h"

#include "core/type.h"
#include "gpu/render_graph.h"

namespace Demo {

	struct Scene;

	namespace UI {
		struct SoulImTexture {
		public:

			union Val {

				soul::gpu::TextureNodeID renderGraphTex = soul::gpu::TEXTURE_NODE_ID_NULL;
				ImTextureID imTextureID;

				Val() {};
			} val;

			static_assert(sizeof(ImTextureID) <= sizeof(val), "");

			SoulImTexture() = default;


			SoulImTexture(soul::gpu::TextureNodeID texNodeID) {
				val.renderGraphTex = texNodeID;
			}

			SoulImTexture(ImTextureID imTextureID) {
				val.imTextureID = imTextureID;
			}

			ImTextureID getImTextureID() {
				return val.imTextureID;
			}

			soul::gpu::TextureNodeID getTextureNodeID() {
				return val.renderGraphTex;
			}

		};
		static_assert(sizeof(SoulImTexture) <= sizeof(ImTextureID), "Size of SoulImTexture must be less that ImTextureID");

		struct Store {
			Scene* scene;
			soul::gpu::System* gpuSystem;

			SoulImTexture sceneTex;
			SoulImTexture fontTex;
			
			ScenePanel scenePanel;
			MetricPanel metricPanel;
			MenuBar menuBar;
		};
	}
}