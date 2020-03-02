#pragma once

namespace Soul { namespace Memory {

	class Allocator;
	class Profiler;

	template <typename PROXY1, typename PROXY2, typename PROXY3, typename PROXY4, typename PROXY5>
	class MultiProxy;
	class NoOpProxy;
	class CounterProxy;
	class ClearValuesProxy;
	class BoundGuardProxy;

	template<typename BACKING_ALLOCATOR,
			typename PROXY>
	class ProxyAllocator;
	class MTLinearAllocator;
	class MallocAllocator;

	using TempProxy = NoOpProxy;
	using TempAllocator = ProxyAllocator<MTLinearAllocator, NoOpProxy>;

	using DefaultAllocatorProxy = Memory::MultiProxy<
			Memory::CounterProxy,
			Memory::ClearValuesProxy,
			Memory::BoundGuardProxy,
			Memory::NoOpProxy,
			Memory::NoOpProxy>;
	using DefaultAllocator = Memory::ProxyAllocator<
			Memory::MallocAllocator,
			DefaultAllocatorProxy>;

	class System {
	public:

		void setDefaultAllocator(DefaultAllocator* defaultAllocator);
		void setTempAllocator(TempAllocator* tempAllocator);
		void cleanup();

		inline static System& Get() {
			static System instance;
			return instance;
		}

		DefaultAllocator* getDefaultAllocator();
		TempAllocator* getTempAllocator();

		Profiler* getProfiler();

	private:

		DefaultAllocator* _defaultAllocator = nullptr;
		TempAllocator*  _tempAllocator = nullptr;
		Allocator* _profilerAllocator = nullptr;
		Profiler* _profiler = nullptr;
		System() = default;

	};

}}