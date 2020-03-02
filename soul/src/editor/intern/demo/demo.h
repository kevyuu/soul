#include "core/pool.h"
#include "editor/data.h"

namespace Soul {
	namespace Editor {
		

		struct Demo {

			virtual void init(Database* db) = 0;
			virtual void tick(Database* db) = 0;
			virtual void cleanup(Database* db) = 0;
		
		};
	
		struct SeaOfLightDemo : Demo {

			Array<EntityID> lightBalls;
			Array<float> velocities;
			EntityID sponza;

			virtual void init(Database* db);
			virtual void tick(Database* db);
			virtual void cleanup(Database* db);

		};


	}
}