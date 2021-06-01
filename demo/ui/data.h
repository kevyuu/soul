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

				Soul::GPU::TextureNodeID renderGraphTex = Soul::GPU::TEXTURE_NODE_ID_NULL;
				ImTextureID imTextureID;

				Val() {};
			} val;

			static_assert(sizeof(ImTextureID) <= sizeof(val), "");

			SoulImTexture() = default;


			SoulImTexture(Soul::GPU::TextureNodeID texNodeID) {
				val.renderGraphTex = texNodeID;
			}

			SoulImTexture(ImTextureID imTextureID) {
				val.imTextureID = imTextureID;
			}

			ImTextureID getImTextureID() {
				return val.imTextureID;
			}

			Soul::GPU::TextureNodeID getTextureNodeID() {
				return val.renderGraphTex;
			}

		};
		static_assert(sizeof(SoulImTexture) <= sizeof(ImTextureID), "Size of SoulImTexture must be less that ImTextureID");

		struct Store {
			Scene* scene;
			Soul::GPU::System* gpuSystem;

			SoulImTexture sceneTex;
			SoulImTexture fontTex;
			
			ScenePanel scenePanel;
			MetricPanel metricPanel;
			MenuBar menuBar;
		};
	}
}