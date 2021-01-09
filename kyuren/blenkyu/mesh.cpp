#include "mesh.h"
#include "core/dev_util.h"

namespace Blender {
#include "RNA_define.h"
#include "rna_internal_types.h"
}

namespace BlenKyu {

	template<typename CustomData>
	static int CustomData_get_active_layer_index(const CustomData* data, int type)
	{
		const int layer_index = data->typemap[type];
		return (layer_index != -1) ? layer_index + data->layers[layer_index].active : -1;
	}

	template<typename CustomData>
	static void* CustomData_get_layer(const CustomData* data, int type)
	{
		/* get the layer index of the active layer of type */
		int layer_index = CustomData_get_active_layer_index(data, type);
		if (layer_index == -1) {
			return nullptr;
		}

		return data->layers[layer_index].data;
	}

	Blender::FunctionRNA* Mesh::CalcNormalsSplit = nullptr;
	
	void Mesh::Init(Blender::StructRNA* structRNA) {

	}

	Mesh::Mesh(void* blenderPtr) { _blenderMesh = (Blender::Mesh*) blenderPtr; }

	MeshIndexList Mesh::indexes() { return { _blenderMesh }; }

	uint32 MeshIndexList::count() {
		return _blenderMesh->runtime.looptris.len * 3;
	}

	uint32 MeshIndexList::operator[](int index) const {
		Blender::MLoopTri* loopTri = _blenderMesh->runtime.looptris.array;
		return loopTri[index / 3].tri[index % 3];
	}

	MeshVertexList Mesh::vertices() { return { _blenderMesh }; }

	uint32 MeshVertexList::count() {
		return _blenderMesh->totloop;
	}

	Vertex MeshVertexList::operator[](int index) const {
		uint32 vertIndex = _blenderMesh->mloop[index].v;
		
		float(*loopNormals)[3] = (float(*)[3]) CustomData_get_layer(&_blenderMesh->ldata, Blender::CD_NORMAL);

		Vertex vertex;
		Blender::MVert mvert = _blenderMesh->mvert[vertIndex];
		vertex.pos = { mvert.co[0], mvert.co[1], mvert.co[2] };
		vertex.normal = { loopNormals[index][0], loopNormals[index][1], loopNormals[index][2] };
		return vertex;
	}

}