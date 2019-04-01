#include "extern/ImGuizmo.h"
#include "extern/imgui.h"

#include "core/math.h"

#include "editor/data.h"
#include "editor/intern/entity.h"

namespace Soul {
	namespace Editor {
		void Manipulator::tick(Database* db) {
			
			Render::Camera& camera = db->world.camera;

			if (db->selectedEntity == db->world.rootEntityID) {
				// do nothing
			} else {
				Entity* entity = EntityPtr(&db->world, db->selectedEntity);
				Mat4 entityMat = mat4Transform(entity->worldTransform);
				Mat4 entityMatTranspose = mat4Transpose(entityMat);
				ImGuizmo::SetRect(0, 0, camera.viewportWidth, camera.viewportHeight);
				ImGuizmo::Manipulate(mat4Transpose(camera.view).mem, mat4Transpose(camera.projection).mem, operation, mode, entityMatTranspose.mem);
				Mat4 entityMatAfter = mat4Transpose(entityMatTranspose);
				if (entityMatAfter != entityMat) {
					Transform entityTransformAfter = transformMat4(entityMatAfter);
					EntitySetWorldTransform(&db->world, entity, entityTransformAfter);
				}
			}
		}
	}
}