//
// Created by Kevin Yudi Utama on 6/2/20.
//

#include "core/math.h"
#include "memory/profiler.h"

namespace soul { namespace memory {

	uint64 HashAddr(const void* addr) {
		const auto uintAddr = uint64(addr);
		return hash_fnv1((const uint8*) &uintAddr, sizeof(void*));
	}

	void Profiler::init() {
		_allocatorsData.reserve(8);
	}

	void Profiler::cleanup() {
		_allocatorsData.cleanup();
	}

	void Profiler::registerAllocator(const char* allocatorName) {
		uint64 hashKey = HashAddr(allocatorName);
		SOUL_ASSERT(0, !_allocatorsData.isExist(hashKey), "");
		PackedID packedID = _allocatorNames.add(allocatorName);
		_allocatorsData.add(hashKey, AllocatorData(_allocator));
		_allocatorsData[hashKey].init();
		_allocatorsData[hashKey].index = packedID;
	};

	void Profiler::unregisterAllocator(const char* allocatorName) {
		uint64 hashKey = HashAddr(allocatorName);
		PackedID packedID = _allocatorsData[hashKey].index;
		_allocatorsData[hashKey].cleanup();
		_allocatorsData.remove(hashKey);
		_allocatorNames.remove(packedID);
	}

	void Profiler::registerAllocation(const char* allocatorName, const char* tag, const void* addr, uint32 size) {
		AllocatorData& allocatorData = _allocatorsData[HashAddr(allocatorName)];
		PackedID allocIndex = allocatorData.allocationTags.add(tag);
		allocatorData.allocations.add(HashAddr(addr), {tag, addr, size, allocIndex});
		if (_allocatorsData.isExist(HashAddr(tag))) {
			AllocatorData& allocData = _allocatorsData[HashAddr(tag)];
			PackedID regionPackID = allocData.regionAddrs.add(addr);
			allocData.regions.add(HashAddr(addr), {addr, size, regionPackID});
		}
	}

	void Profiler::registerDeallocation(const char* allocatorName, const void* addr, uint32 size) {
		AllocatorData& allocatorData = _allocatorsData[HashAddr(allocatorName)];
		const Allocation& allocation = allocatorData.allocations[HashAddr(addr)];
		if (_allocatorsData.isExist(HashAddr(allocation.tag))) {
			UInt64HashMap<Region>& regions = _allocatorsData[HashAddr(allocation.tag)].regions;
			_allocatorsData[HashAddr(allocation.tag)].regionAddrs.remove(regions[HashAddr(addr)].index);
			regions.remove(HashAddr(addr));
		}
		allocatorData.allocationTags.remove(allocation.index);
		allocatorData.allocations.remove(HashAddr(addr));
	}

	void Profiler::snapshot(const char* name) {
		Array<Snapshot>& snapshots = _frames.back().snapshots;
		snapshots.add(Snapshot(name, _allocatorsData, _allocatorNames));
	}

	const Profiler::Region& Profiler::AllocatorData::getRegion(const void* addr) const {
		return regions[HashAddr(addr)];
	}

	bool Profiler::AllocatorData::isAllocationExist(const char *allocationName) const {
		return allocations.isExist(HashAddr(allocationName));
	}

	const Profiler::Allocation& Profiler::AllocatorData::getAllocation(const char *allocationName) const {
		return allocations[HashAddr(allocationName)];
	}

	bool Profiler::Snapshot::isAllocatorDataExist (const char *allocatorNames) const {
		return allocatorsData.isExist(HashAddr(allocatorNames));
	}

	const Profiler::AllocatorData& Profiler::Snapshot::getAllocatorData(const char *allocatorNames) const {
		return allocatorsData[HashAddr(allocatorNames)];
	}

}}