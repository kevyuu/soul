#pragma once

#include "editor/data.h"

namespace Soul {
	namespace Editor {

		struct System {


			void init();
			void shutdown();

			void run();
			void tick();

			Database _db;
		};
		

	}
}