#pragma once

#include "core/util.h"
#include "object.h"

namespace Blender {

	#include "MEM_guardedalloc.h"
	#include "RNA_define.h"
	#include "RNA_access.h"
	#include "RNA_types.h"
	#include "rna_internal_types.h"
	#include "BLI_iterator.h"
	#include "DEG_depsgraph_query.h"
	#include "BKE_duplilist.h"
	#include "DNA_light_types.h"
	struct CollectionPropertyRNA;
	struct StructRNA;
	struct Depsgraph;
}

namespace BlenKyu {

	class Object;

	class Depsgraph {
		
	public:

		struct Instance {
			float* matrixWorld;
			Object* obj;
		};

		static void Init(Blender::StructRNA* structRNA);

		static Blender::CollectionPropertyRNA* ObjectInstancesProperty;

		Depsgraph(void* ptr);

		template <SOUL_TEMPLATE_ARG_LAMBDA(T, void(Instance))>
		void forEachObjectInstance(const T& func) {
			Blender::CollectionPropertyIterator iterator;
			Blender::PointerRNA depsgraphPointerRNA;
			depsgraphPointerRNA.data = _blenderDepsgraph;

			ObjectInstancesProperty->begin(&iterator, &depsgraphPointerRNA);
			while (iterator.valid) {
				auto bliIter = (Blender::BLI_Iterator*)ObjectInstancesProperty->get(&iterator).data;
				auto blenObj = (Blender::Object*)bliIter->current;
				auto properties = blenObj->id.properties;

				float* matrixWorld = blenObj->obmat[0];

				auto degIter = (Blender::DEGObjectIterData*)bliIter->data;
				if (degIter->dupli_object_current != NULL) {
					matrixWorld = degIter->dupli_object_current->mat[0];
				}

				Object blenKyuObj(blenObj);
				Instance instance = { matrixWorld, &blenKyuObj };
				func(instance);

				ObjectInstancesProperty->next(&iterator);
			}
			ObjectInstancesProperty->end(&iterator);
		}

	private:
		Blender::Depsgraph* _blenderDepsgraph;
	};
}