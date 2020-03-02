#include "memory_profiler_panel.h"

#include "core/type.h"
#include "core/math.h"
#include "core/array.h"

#include "imgui/imgui.h"

#include "memory/memory.h"
#include "memory/profiler.h"

using Frame = Soul::Memory::Profiler::Frame;
using Snapshot = Soul::Memory::Profiler::Snapshot;
using AllocatorData = Soul::Memory::Profiler::AllocatorData;
using Region = Soul::Memory::Profiler::Region;

MemoryProfilerPanel::MemoryProfilerPanel() {
	_selectedFrame = 0;
	_selectedSnapshot = 0;
	_selectedAllocator = nullptr;
	_firstFrame = true;
	_scale = 10.24f;
	_startMemSpace = 0;
}

ImVec2 operator+(ImVec2 left, ImVec2 right) {
	return ImVec2(left.x + right.x, left.y + right.y);
}

void MemorySpaceToString(uint64 memorySpace, char* buf) {
	static const uint64 ONE_TERRA = 1024ll * 1024 * 1024 * 1024;
	static const uint64 ONE_GIGA = 1024ll * 1024 * 1024;
	static const uint64 ONE_MEGA = 1024ll * 1024;
	static const uint64 ONE_KILO = 1024ll;

	if (memorySpace >= ONE_TERRA && memorySpace % ONE_TERRA == 0) {
		sprintf(buf, "%lld %s", memorySpace / ONE_TERRA, "TiB");
	} else if (memorySpace >= ONE_GIGA && memorySpace % ONE_GIGA == 0) {
		sprintf(buf, "%lld %s", memorySpace / ONE_GIGA, "GiB");
	} else if (memorySpace >= ONE_MEGA && memorySpace % ONE_MEGA == 0) {
		sprintf(buf, "%lld %s", memorySpace / ONE_MEGA, "MiB");
	} else if (memorySpace >= ONE_KILO && memorySpace % ONE_KILO == 0) {
		sprintf(buf, "%lld %s", memorySpace / ONE_KILO, "KiB");
	} else {
		sprintf(buf, "%lld %s", memorySpace, "B");
	}

}

void MemoryProfilerPanel::update() {
	SOUL_NOT_IMPLEMENTED();
	const Soul::Memory::Profiler* profiler = nullptr;

	ImGui::Begin("Memory Profiler");

	{
		ImGui::Columns(4);
		if (_firstFrame) {
			ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() / 16);
			ImGui::SetColumnWidth(1, ImGui::GetWindowWidth() / 8);
			ImGui::SetColumnWidth(2, ImGui::GetWindowWidth() / 8);
			_firstFrame = false;
		}

		ImGui::Text("Frames");
		ImGui::NextColumn();

		ImGui::Text("Snapshots");
		ImGui::NextColumn();

		ImGui::Text("Allocators");
		ImGui::NextColumn();

		ImGui::Text("Regions");
		ImGui::NextColumn();

		ImGui::Separator();
		const Soul::Array<Frame>& frames = profiler->getFrames();

		{
			ImGui::BeginChild("Frames");
			for (int i = 0; i < frames.size(); i++) {
				char buf[100];
				sprintf(buf, "Frames %d", i);
				if (ImGui::Selectable(buf, _selectedFrame == i))
					_selectedFrame = i;
			}
			ImGui::EndChild();
		}
		ImGui::NextColumn();


		if (_selectedFrame < frames.size()){
			const Soul::Array<Snapshot>& snapshots = frames[_selectedFrame].snapshots;
			ImGui::BeginChild("Snapshots");
			for (int i = 0; i < snapshots.size(); i++) {
				if (ImGui::Selectable(snapshots[i].name, _selectedSnapshot == i))
					_selectedSnapshot = i;
			}
			ImGui::EndChild();
			ImGui::NextColumn();

			if (_selectedSnapshot < snapshots.size()) {
				const Snapshot& snapshot = snapshots[_selectedSnapshot];
				const Soul::PackedPool<const char*>& allocatorNames = snapshot.allocatorNames;
				ImGui::BeginChild("Allocators");
				for (const char* allocatorName : allocatorNames) {
					if (ImGui::Selectable(allocatorName, _selectedAllocator == allocatorName))
						_selectedAllocator = allocatorName;
				}
				ImGui::EndChild();
				ImGui::NextColumn();

				if (snapshot.isAllocatorDataExist(_selectedAllocator)) {
					const AllocatorData& allocatorData = snapshot.getAllocatorData(_selectedAllocator);

					ImGui::BeginChild("Regions", ImVec2(0, 0), false, _noRegionScroll ? ImGuiWindowFlags_NoScrollWithMouse : 0);
					if (ImGui::IsMouseDown(1)) {
						_noRegionScroll = true;

						_scale += ImGui::GetIO().MouseWheel;
						_scale = std::max<float>(1, _scale);
						if (ImGui::IsMouseDragging(1, 0)) {
							ImVec2 delta = ImGui::GetMouseDragDelta(1, 0);
							_startMemSpace = _startMemSpace - std::min<int64>(_startMemSpace, int64(_scale * delta.x));
						}
					} else {
						_noRegionScroll = false;
					}
					ImDrawList* drawList = ImGui::GetWindowDrawList();
					const ImVec2 p = ImGui::GetCursorScreenPos();

					float rulerWidth = ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ScrollbarSize - 180;
					ImVec2 tickStartPos = ImVec2(p.x + 180.0f, p.y);
					float tickX = 0;
					float tickHeight = ImGui::GetFontSize();
					uint64 memoryStep = std::max<uint64>(10, Soul::roundToNextPowOfTwo(_scale * 100));
					float tickStep = memoryStep / _scale;

					float tickSubSpace = tickStep / 10.0f;
					uint64 memoryCurrent = 0;

					while (tickX < rulerWidth) {
						drawList->AddLine(tickStartPos + ImVec2(tickX, 0), tickStartPos + ImVec2( tickX, round( tickHeight * 0.5 ) ), 0x66FFFFFF );
						char buf[20];
						if (memoryCurrent == 0) {
							MemorySpaceToString(uint64(_startMemSpace), buf + 1);
							buf[0] = '+';
							drawList->AddText( tickStartPos + ImVec2( tickX, round( ImGui::GetFontSize() * 0.5 ) ), 0x66FFFFFF, buf );
						} else {
							MemorySpaceToString( uint64(memoryCurrent), buf);
							drawList->AddText( tickStartPos + ImVec2( tickX, round( ImGui::GetFontSize() * 0.5 ) ), 0x66FFFFFF, buf );
						}
						for( int i=1; i<5; i++ )
						{
							drawList->AddLine( tickStartPos + ImVec2( tickX + i * tickSubSpace, 0 ), tickStartPos + ImVec2( tickX + i * tickSubSpace, round( tickHeight * 0.25 ) ), 0x33FFFFFF );
						}
						drawList->AddLine( tickStartPos + ImVec2( tickX + 5 * tickSubSpace, 0 ), tickStartPos + ImVec2( tickX + 5 * tickSubSpace, round( tickHeight * 0.375 ) ), 0x33FFFFFF );
						for( int i=6; i<10; i++ )
						{
							drawList->AddLine( tickStartPos + ImVec2( tickX + i * tickSubSpace, 0 ), tickStartPos + ImVec2( tickX + i * tickSubSpace, round( tickHeight * 0.25 ) ), 0x33FFFFFF );
						}
						tickX += tickStep;
						memoryCurrent += memoryStep;
					}

					float regionTextX = p.x + 4.0f;
					float regionBarX = p.x + 180.0f;
					float y = p.y + 30.0f;
					const ImU32 col = ImColor(0.2f, 1.0f, 0.2f, 1.0f);
					for (const void* regionAddr: allocatorData.regionAddrs) {
						const Region& region = allocatorData.getRegion(regionAddr);
						char memSizeStr[10];
						MemorySpaceToString(region.size, memSizeStr);
						char label[50];
						sprintf(label, "%p (%s)", region.addr, memSizeStr);
						drawList->AddText(ImVec2(regionTextX, y), ImColor(1.0f, 1.0f, 1.0f, 1.0f), label);
						drawList->AddRectFilled(ImVec2(regionBarX, y), ImVec2(regionBarX +  std::max<int64>((region.size - _startMemSpace), 0) / _scale, y + 20.0f), col);
						y += 24.0f;
					}
					ImGui::Dummy(ImVec2(ImGui::GetWindowWidth(), y - p.y));
					ImGui::EndChild();
				}

			}
		}

		ImGui::End();


	}

	ImGui::End();
}