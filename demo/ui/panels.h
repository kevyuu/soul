#pragma once

#include "core/type.h"
#include "core/math.h"

#include "imgui/imgui.h"

using namespace Soul;

namespace Demo {
	namespace UI {
		struct Store;

		struct ScenePanel {

			Vec2ui32 sceneResolution;
			const char* name = "Scene";
			ImTextureID textureID;

			void setResolution(Vec2ui32 sceneResolution) { this->sceneResolution = sceneResolution; };
			void setName(const char* name) { this->name = name; }
			void setTexture(ImTextureID textureID) { this->textureID = textureID; }
			void render(Store* store);

		};

		struct SceneConfigPanel {
			void render(Store* store);
		};

		struct MetricPanel {
			void render(Store* store);
		};
	}
}
