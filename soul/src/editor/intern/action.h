#pragma once

#include "editor/data.h"

namespace Soul {
	namespace Editor {
		EntityID ActionImportGLTFAsset(World* world, const char* path, bool positionToAABBCenter);
	}
}