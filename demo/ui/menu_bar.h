#pragma once

namespace Demo {
	namespace UI {

		struct Store;

		struct MenuBar {

			char gltfFilePath[1000] = {};
			void render(Store* store);

		};

	}
}