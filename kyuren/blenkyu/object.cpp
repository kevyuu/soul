#include "object.h"
#include "core/dev_util.h"
#include "core/uint64_hash_map.h"
#include <string.h>

namespace Blender {
	#include "DNA_ID.h"
	#include "DNA_object_types.h"
}

namespace BlenKyu {

	Object::Object(void* ptr) {
		_blenderObject = (Blender::Object*) ptr;
	}

	uint64 Object::id() {
		return uint64(_blenderObject->id.orig_id);
	}

	bool Object::isMesh() {
		return _blenderObject->type == Blender::OB_MESH;
	}

	bool Object::isLight() {
		return _blenderObject->type == Blender::OB_LAMP;
	}
}