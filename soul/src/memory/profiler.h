#pragma once

#include "core/array.h"
#include "core/uint64_hash_map.h"

#include "core/packed_pool.h"

namespace Soul {
	namespace Memory {

		class Allocator;
		class Profiler {
		public:
			struct Region {
				const void* addr;
				uint32 size;
				uint32 index;
			};

			struct Allocation {
				const char* tag;
				const void* addr;
				uint32 size;
				uint32 index;
			};

			struct AllocatorData {
				uint32 index;

				UInt64HashMap<Region> regions;
				PackedPool<const void*> regionAddrs;

				UInt64HashMap<Allocation> allocations;
				PackedPool<const char*> allocationTags;

				explicit AllocatorData(Allocator* allocator) :
					regions(allocator), regionAddrs(allocator),
					allocations(allocator),  allocationTags(allocator) {}

				void init() {
					SOUL_ASSERT(0, regionAddrs.capacity() == 0, "");
					SOUL_ASSERT(0, allocationTags.capacity() == 0, "");
					regions.reserve(8);
					allocations.reserve(8);
				}

				AllocatorData(const AllocatorData& other) : index(other.index),
					regionAddrs(other.regionAddrs), regions(other.regions),
					allocations(other.allocations), allocationTags(other.allocationTags) {

				}

				const Region& getRegion(const void* regionAddr) const;

				const Allocation& getAllocation(const char* allocationName) const;
				bool isAllocationExist (const char* allocationName) const;

				void cleanup() {
					regions.cleanup();
					regionAddrs.cleanup();

					allocations.cleanup();
					allocationTags.cleanup();
				}
			};

			struct Snapshot {
				const char* name;
				UInt64HashMap<AllocatorData> allocatorsData;
				PackedPool<const char*> allocatorNames;

				explicit Snapshot(const char* name,
								  const UInt64HashMap<AllocatorData>& allocatorsData,
								  const PackedPool<const char*>& allocatorNames):
						name(name), allocatorsData(allocatorsData), allocatorNames(allocatorNames) {

				}

				const AllocatorData& getAllocatorData(const char* allocatorNames) const;
				bool isAllocatorDataExist(const char* allocatorNames) const;
			};

			struct Frame {
				Array<Snapshot> snapshots;
				Frame(Allocator* allocator) : snapshots(allocator) {}
			};

			explicit Profiler(Allocator* allocator):
				_allocator(allocator),
				_allocatorsData(allocator),
				_allocatorNames(allocator),
				_frames(allocator) {}

			void init();
			void cleanup();

			void registerAllocator(const char* allocatorName);
			void unregisterAllocator(const char* allocatorName);
			void registerAllocation(const char* allocatorName, const char* tag, const void* addr, uint32 size);
			void registerDeallocation(const char* allocatorName, const void* addr, uint32 size);
			void beginFrame() { _frames.add(Frame(_allocator)); }
			void endFrame() {}
			void snapshot(const char* name);

			const Array<Frame>& getFrames() const { return _frames; }

		private:
			Allocator* _allocator;
			UInt64HashMap<AllocatorData> _allocatorsData;
			PackedPool<const char*> _allocatorNames;
			Array<Frame> _frames;
		};

	}
}