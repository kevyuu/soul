#pragma once

#include "core/type.h"

struct MemoryProfilerPanel {
	uint32 _selectedFrame;
	uint32 _selectedSnapshot;
	const char* _selectedAllocator;
	bool _firstFrame;
	float _scale;
	bool _noRegionScroll;
	uint64 _startMemSpace;

	MemoryProfilerPanel();
	void update();

};