//
// Created by Kevin Yudi Utama on 6/2/20.
//

#include "core/math.h"
#include "memory/profiler.h"

namespace Soul { namespace Memory {

	uint64 _HashAddr(const void* addr) {
		uint64 uintAddr = uint64(addr);
		return hashFNV1((const uint8*) &uintAddr, sizeof(void*));
	}

	void Profiler::init() {
		_allocatorsData.reserve(8);
	}

	void Profiler::cleanup() {
		_allocatorsData.cleanup();
	}

	void Profiler::registerAllocator(const char* allocatorName) {
		uint64 hashKey = _HashAddr(allocatorName);
		SOUL_ASSERT(0, !_allocatorsData.isExist(hashKey), "");
		PackedID packedID = _allocatorNames.add(allocatorName);
		_allocatorsData.add(hashKey, AllocatorData(_allocator));
		_allocatorsData[hashKey].init();
		_allocatorsData[hashKey].index = packedID;
	};

	void Profiler::unregisterAllocator(const char* allocatorName) {
		uint64 hashKey = _HashAddr(allocatorName);
		PackedID packedID = _allocatorsData[hashKey].index;
		_allocatorsData[hashKey].cleanup();
		_allocatorsData.remove(hashKey);
		_allocatorNames.remove(packedID);
	}

	void Profiler::registerAllocation(const char* allocatorName, const char* tag, const void* addr, uint32 size) {
		AllocatorData& allocatorData = _allocatorsData[_HashAddr(allocatorName)];
		PackedID allocIndex = allocatorData.allocationTags.add(tag);
		allocatorData.allocations.add(_HashAddr(addr), {tag, addr, size, allocIndex});
		if (_allocatorsData.isExist(_HashAddr(tag))) {
			AllocatorData& allocData = _allocatorsData[_HashAddr(tag)];
			PackedID regionPackID = allocData.regionAddrs.add(addr);
			allocData.regions.add(_HashAddr(addr), {addr, size, regionPackID});
		}
	}

	void Profiler::registerDeallocation(const char* allocatorName, const void* addr, uint32 size) {
		AllocatorData& allocatorData = _allocatorsData[_HashAddr(allocatorName)];
		const Allocation& allocation = allocatorData.allocations[_HashAddr(addr)];
		if (_allocatorsData.isExist(_HashAddr(allocation.tag))) {
			HashMap<Region>& regions = _allocatorsData[_HashAddr(allocation.tag)].regions;
			_allocatorsData[_HashAddr(allocation.tag)].regionAddrs.remove(regions[_HashAddr(addr)].index);
			regions.remove(_HashAddr(addr));
		}
		allocatorData.allocationTags.remove(allocation.index);
		allocatorData.allocations.remove(_HashAddr(addr));
	}

	void Profiler::snapshot(const char* name) {
		Array<Snapshot>& snapshots = _frames.back().snapshots;
		snapshots.add(Snapshot(name, _allocatorsData, _allocatorNames));
	}

	const Profiler::Region& Profiler::AllocatorData::getRegion(const void* addr) const {
		return regions[_HashAddr(addr)];
	}

	bool Profiler::AllocatorData::isAllocationExist(const char *allocationName) const {
		return allocations.isExist(_HashAddr(allocationName));
	}

	const Profiler::Allocation& Profiler::AllocatorData::getAllocation(const char *allocationName) const {
		return allocations[_HashAddr(allocationName)];
	}

	bool Profiler::Snapshot::isAllocatorDataExist (const char *allocatorNames) const {
		return allocatorsData.isExist(_HashAddr(allocatorNames));
	}

	const Profiler::AllocatorData& Profiler::Snapshot::getAllocatorData(const char *allocatorNames) const {
		return allocatorsData[_HashAddr(allocatorNames)];
	}

}}