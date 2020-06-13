#include "memory/allocators/mt_linear_allocator.h"
#include "runtime/runtime.h"

namespace Soul {namespace Memory {

	MTLinearAllocator::MTLinearAllocator(const char* name, uint32 sizePerThread, Allocator* backingAllocator):
		Allocator(name), _backingAllocator(backingAllocator)
	{
		SOUL_ASSERT_MAIN_THREAD();

		_threadCount = Runtime::System::Get().getThreadCount();
		_sizePerThread = sizePerThread;
		Allocation allocation = _backingAllocator->allocate(
				(sizeof(PerThread) + _sizePerThread) * _threadCount, alignof(PerThread), getName());
		SOUL_ASSERT(0, allocation.addr != nullptr, "");
		_totalSize = allocation.size;
		_perThreads = (PerThread*) allocation.addr;

		void* threadDataStartAddr = Util::Add(allocation.addr, sizeof(PerThread) * _threadCount);
		for (int i = 0; i < _threadCount; i++) {
			_perThreads[i].baseAddr = Util::Add(threadDataStartAddr, i * _sizePerThread);
			_perThreads[i].currentAddr = _perThreads[i].baseAddr;
		}
	}

	void MTLinearAllocator::reset() {}

	Allocation MTLinearAllocator::allocate(uint32 size, uint32 alignment, const char* tag) {

		PerThread& perThread = _perThreads[Runtime::System::Get().getThreadID()];
		void* addr = Util::AlignForward(perThread.currentAddr, alignment);
		if (Util::Add(addr, size) > Util::Add(perThread.baseAddr, _sizePerThread)) {
			return { nullptr, 0 };
		}
		perThread.currentAddr = Util::Add(addr, size);
		return { addr, size };
	}

	void MTLinearAllocator::deallocate(void* addr, uint32 size) {}

	void* MTLinearAllocator::getMarker() {
		PerThread& perThread = _perThreads[Runtime::System::Get().getThreadID()];
		return perThread.currentAddr;
	}

	void MTLinearAllocator::rewind(void* addr) {
		PerThread& perThread = _perThreads[Runtime::System::Get().getThreadID()];
		SOUL_ASSERT(0, addr >= perThread.baseAddr && addr <= perThread.currentAddr, "");
		perThread.currentAddr = addr;
	}

	MTLinearAllocator::~MTLinearAllocator() {
		_backingAllocator->deallocate(_perThreads, _totalSize);
	}

}}