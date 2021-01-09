#pragma once

#include "core/type.h"

namespace Blender {
	struct Object;
}

namespace Soul {
	template<typename VALUETYPE>
	class UInt64HashMap;
}

namespace BlenKyu {
	class Object {
	public:

		Object(void* blenderPtr);

		static const int KYUREN_ID_NULL = -1;

		bool isMesh();
		bool isLight();
		uint64 id();
	
	private:
		Blender::Object* _blenderObject;
	};
}