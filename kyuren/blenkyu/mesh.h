#pragma once

#include "core/type.h"

namespace Blender {
	#include "DNA_meshdata_types.h"
	#include "DNA_mesh_types.h"
	struct StructRNA;
	struct FunctionRNA;
}

namespace BlenKyu {

	struct Vertex {
		Soul::Vec3f pos;
		Soul::Vec3f normal;
	};

	struct MeshIndexList {
		Blender::Mesh* _blenderMesh;
		uint32 operator[](int index) const;
		uint32 count();
	};

	struct MeshVertexList {
		Blender::Mesh* _blenderMesh;
		Vertex operator[](int index) const;
		uint32 count();
	};

	class Mesh {

	public:
		static void Init(Blender::StructRNA* structRNA);
		Mesh::Mesh(void* blenderPtr);
		MeshIndexList indexes();
		MeshVertexList vertices();

	private:
		Blender::Mesh* _blenderMesh;
		static Blender::FunctionRNA* CalcNormalsSplit;
	};
}