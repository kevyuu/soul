#include "depsgraph.h"
#include <string.h>

namespace Blender {
#include "RNA_define.h"
#include "rna_internal_types.h"
}

namespace BlenKyu {
	
	Blender::CollectionPropertyRNA* Depsgraph::ObjectInstancesProperty = nullptr;

	void Depsgraph::Init(Blender::StructRNA* structRNA) {
		for (Blender::PropertyRNA* property = (Blender::PropertyRNA*)structRNA->cont.properties.first; property;  property = property->next) {
			if (strcmp(property->identifier, "object_instances") == 0) {
				Depsgraph::ObjectInstancesProperty = (Blender::CollectionPropertyRNA*)property;
			}
		}
		SOUL_ASSERT(0, Depsgraph::ObjectInstancesProperty != nullptr, "");
	}

	Depsgraph::Depsgraph(void* blenderPtr) {
		_blenderDepsgraph = (Blender::Depsgraph*)blenderPtr;
	}

}