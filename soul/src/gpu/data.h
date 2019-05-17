#include "core/type.h"
#include "core/debug.h"

namespace Soul {
	namespace GPU {

		typedef uint32 VertexBufferID;
		typedef uint32 IndexBufferID;
		
		namespace Command {

			struct DrawIndex {
				VertexBufferID vertexBufferID;
				IndexBufferID indexBufferID;
			};

			struct DrawBuffer {

			};

		} 

	}
}