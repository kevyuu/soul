#pragma once

#include "core/type.h"

namespace Demo {
	namespace UI {
        int PiePopupSelectMenu(const char* popup_id, const char** items, int items_count, int triggerKey);

		bool BeginPiePopup(const char* pName, int iMouseButton = 0);
		void EndPiePopup();

		bool PieMenuItem(const char* pName, bool bEnabled = true);
		bool BeginPieMenu(const char* pName, bool bEnabled = true);
		void EndPieMenu();
	}
}