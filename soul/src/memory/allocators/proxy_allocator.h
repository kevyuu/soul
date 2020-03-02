#pragma once

#include "memory/allocator.h"

namespace Soul { namespace Memory {

	struct AllocateParam {
		uint32 size;
		uint32 alignment;
		const char* tag;
	};

	struct DeallocateParam {
		void* addr;
		uint32 size;
	};

	class Proxy {
	public:

		virtual void onPreInit(const char* name) = 0;
		virtual void onPostInit() = 0;

		virtual AllocateParam onPreAllocate(const AllocateParam& allocParam) = 0;
		virtual Allocation onPostAllocate(Allocation allocation) = 0;

		virtual DeallocateParam onPreDeallocate(const DeallocateParam& deallocParam) = 0;
		virtual void onPostDeallocate() = 0;

		virtual void onPreCleanup() = 0;
		virtual void onPostCleanup() = 0;
	};

	class NoOpProxy : public Proxy {
	public:
		void onPreInit(const char* name) final {};
		void onPostInit() final {};

		AllocateParam onPreAllocate(const AllocateParam& allocParam) final { return allocParam; };
		Allocation onPostAllocate(Allocation allocation) final {return allocation; };

		DeallocateParam onPreDeallocate(const DeallocateParam& deallocParam) final { return deallocParam; };
		void onPostDeallocate() final {};

		void onPreCleanup() final {};
		void onPostCleanup() final {};
	};

	template <
			typename PROXY1 = NoOpProxy,
			typename PROXY2 = NoOpProxy,
			typename PROXY3 = NoOpProxy,
			typename PROXY4 = NoOpProxy,
			typename PROXY5 = NoOpProxy>
	class MultiProxy : public Proxy {
	private:
		PROXY1 _proxy1;
		PROXY2 _proxy2;
		PROXY3 _proxy3;
		PROXY4 _proxy4;
		PROXY5 _proxy5;

	public:

		MultiProxy() = default;

		template <
				typename CPROXY1 = PROXY1,
				typename CPROXY2 = PROXY2,
				typename CPROXY3 = PROXY3,
				typename CPROXY4 = PROXY4,
				typename CPROXY5 = PROXY5>
		explicit MultiProxy(
				CPROXY1&& proxy1 = NoOpProxy(),
				CPROXY2&& proxy2 = NoOpProxy(),
				CPROXY3&& proxy3 = NoOpProxy(),
				CPROXY4&& proxy4 = NoOpProxy(),
				CPROXY5&& proxy5 = NoOpProxy()) :
				_proxy1(std::forward<CPROXY1>(proxy1)),
				_proxy2(std::forward<CPROXY2>(proxy2)),
				_proxy3(std::forward<CPROXY3>(proxy3)),
				_proxy4(std::forward<CPROXY4>(proxy4)),
				_proxy5(std::forward<CPROXY5>(proxy5)) {}

		virtual void onPreInit(const char* name) final override {
			_proxy1.onPreInit(name);
			_proxy2.onPreInit(name);
			_proxy3.onPreInit(name);
			_proxy4.onPreInit(name);
			_proxy5.onPreInit(name);
		};

		virtual void onPostInit() final override {
			_proxy5.onPostInit();
			_proxy4.onPostInit();
			_proxy3.onPostInit();
			_proxy2.onPostInit();
			_proxy1.onPostInit();
		};

		virtual AllocateParam onPreAllocate(const AllocateParam& allocParam) final override {
			AllocateParam param = allocParam;
			param = _proxy1.onPreAllocate(param);
			param = _proxy2.onPreAllocate(param);
			param = _proxy3.onPreAllocate(param);
			param = _proxy4.onPreAllocate(param);
			param = _proxy5.onPreAllocate(param);
			return param;
		};

		virtual Allocation onPostAllocate(Allocation allocation) final override {
			allocation = _proxy5.onPostAllocate(allocation);
			allocation = _proxy4.onPostAllocate(allocation);
			allocation = _proxy3.onPostAllocate(allocation);
			allocation = _proxy2.onPostAllocate(allocation);
			allocation = _proxy1.onPostAllocate(allocation);
			return allocation;
		};

		virtual DeallocateParam onPreDeallocate(const DeallocateParam& deallocParam) final override {
			DeallocateParam param = deallocParam;
			param = _proxy1.onPreDeallocate(param);
			param = _proxy2.onPreDeallocate(param);
			param = _proxy3.onPreDeallocate(param);
			param = _proxy4.onPreDeallocate(param);
			param = _proxy5.onPreDeallocate(param);
			return param;
		};

		virtual void onPostDeallocate() final override {
			_proxy5.onPostDeallocate();
			_proxy4.onPostDeallocate();
			_proxy3.onPostDeallocate();
			_proxy2.onPostDeallocate();
			_proxy1.onPostDeallocate();
		};

		virtual void onPreCleanup() final override {
			_proxy1.onPreCleanup();
			_proxy2.onPreCleanup();
			_proxy3.onPreCleanup();
			_proxy4.onPreCleanup();
			_proxy5.onPreCleanup();
		};

		virtual void onPostCleanup() final override {
			_proxy5.onPostCleanup();
			_proxy4.onPostCleanup();
			_proxy3.onPostCleanup();
			_proxy2.onPostCleanup();
			_proxy1.onPostCleanup();
		};
	};

	class CounterProxy: public Proxy {
	private:
		uint32 _counter;
	public:

		CounterProxy(): _counter(0) {}
		void onPreInit(const char* name) final{ _counter = 0; }
		void onPostInit() final {}

		AllocateParam onPreAllocate(const AllocateParam& allocParam) final { _counter++; return allocParam; }
		Allocation onPostAllocate(Allocation allocation) final { return allocation; };

		DeallocateParam onPreDeallocate(const DeallocateParam& deallocParam) final { _counter--; return deallocParam; };
		void onPostDeallocate() final {};

		void onPreCleanup() final { SOUL_ASSERT(0, _counter == 0, ""); };
		void onPostCleanup() final {};
	};

	class ClearValuesProxy: public Proxy {
	private:
		const char _onAllocClearValue;
		const char _onDeallocClearValue;

		uint32 _currentAllocSize;
	public:
		explicit ClearValuesProxy(const char onAllocatedClearValue, const char onFreeClearValue)
				: _onAllocClearValue(onAllocatedClearValue), _onDeallocClearValue(onFreeClearValue), _currentAllocSize(0) {}

		void onPreInit(const char* name) final {}
		void onPostInit() final {}

		AllocateParam onPreAllocate(const AllocateParam& allocParam) final {
			_currentAllocSize = allocParam.size;
			return allocParam;
		}
		Allocation onPostAllocate(Allocation allocation) final {
			memset(allocation.addr, _onAllocClearValue, _currentAllocSize);
			return allocation;
		};

		DeallocateParam onPreDeallocate(const DeallocateParam& deallocParam) final {
			memset(deallocParam.addr, _onDeallocClearValue, deallocParam.size);
			return deallocParam;
		};
		void onPostDeallocate() final {};

		void onPreCleanup() final {};
		void onPostCleanup() final {};
	};

	class BoundGuardProxy: public Proxy {
	private:
		static constexpr uint32 GUARD_SIZE = alignof(std::max_align_t);
		static constexpr char GUARD_FLAG = 0xAA;
		uint32 _currentAllocSize;

	public:

		void onPreInit(const char* name) final{}
		void onPostInit() final {}

		AllocateParam onPreAllocate(const AllocateParam& allocParam) final {
			_currentAllocSize = allocParam.size;
			return { allocParam.size + 2 * GUARD_SIZE, GUARD_SIZE, allocParam.tag };
		}

		Allocation onPostAllocate(Allocation allocation) final {
			memset(allocation.addr, GUARD_FLAG, GUARD_SIZE);
			memset(Util::Add(allocation.addr, GUARD_SIZE + _currentAllocSize), GUARD_FLAG, GUARD_SIZE);
			char* baseFront = (char*) allocation.addr;
			return {Util::Add(allocation.addr, GUARD_SIZE), allocation.size - 2 * GUARD_SIZE};
		};

		DeallocateParam onPreDeallocate(const DeallocateParam& deallocParam) final {
			// TODO(kevinyu): Make faster
			char* baseFront = (char*) Util::Sub(deallocParam.addr, GUARD_SIZE);
			char* baseBack = (char*) Util::Add(deallocParam.addr, deallocParam.size);
			for (int i = 0; i < GUARD_SIZE; i++) {
				SOUL_ASSERT(0, baseFront[i] == GUARD_FLAG, "");
				SOUL_ASSERT(0, baseBack[i] == GUARD_FLAG, "");
			}
			return { Util::Sub(deallocParam.addr, GUARD_SIZE), deallocParam.size + 2 * GUARD_SIZE };
		};
		void onPostDeallocate() final {};

		void onPreCleanup() final {};
		void onPostCleanup() final {};
	};

	class ProfileProxy : public Proxy {
	private:
		const char* _name;
		AllocateParam _currentAlloc;
	public:
		void onPreInit(const char* name) final;
		void onPostInit() final {};

		AllocateParam onPreAllocate(const AllocateParam& allocParam) final;
		Allocation onPostAllocate(Allocation allocation) final;

		DeallocateParam onPreDeallocate(const DeallocateParam& deallocParam) final;
		void onPostDeallocate() final {};

		void onPreCleanup() final;
		void onPostCleanup() final {};
	};

	template<typename BACKING_ALLOCATOR,
			typename PROXY = NoOpProxy>
	class ProxyAllocator : public Allocator {
	public:

		BACKING_ALLOCATOR* allocator;

		ProxyAllocator() = delete;

		template <typename CPROXY>
		explicit ProxyAllocator(BACKING_ALLOCATOR* allocator, CPROXY&& proxy) :
				Allocator(allocator->getName()),
				allocator(allocator),
				_proxy(std::forward<CPROXY>(proxy)) {
			_proxy.onPreInit(getName());
			_proxy.onPostInit();
		}
		template <typename CPROXY>
		explicit ProxyAllocator(const char* name, BACKING_ALLOCATOR* allocator, CPROXY&& proxy) :
				Allocator(name),
				allocator(allocator),
				_proxy(std::forward<CPROXY>(proxy)) {
			_proxy.onPreInit(getName());
			_proxy.onPostInit();
		}
		~ProxyAllocator() override = default;

		ProxyAllocator(const ProxyAllocator& other) = delete;
		ProxyAllocator& operator=(const ProxyAllocator& other) = delete;

		ProxyAllocator(ProxyAllocator&& other) = delete;
		ProxyAllocator& operator=(ProxyAllocator&& other) = delete;

		void reset() final {
			_proxy.onPreCleanup();
			_proxy.onPostCleanup();
		}

		Allocation allocate(uint32 size, uint32 alignment, const char* tag) final {
			AllocateParam param = { size, alignment, tag };
			param = _proxy.onPreAllocate(param);
			Allocation allocation = allocator->allocate(param.size, param.alignment, getName());
			return _proxy.onPostAllocate(allocation);
		}

		void deallocate(void* addr, uint32 size) final {
			if (addr == nullptr) return;
			DeallocateParam param = { addr, size };
			param = _proxy.onPreDeallocate(param);
			allocator->deallocate(param.addr, size);
			_proxy.onPostDeallocate();
		}

	private:
		PROXY _proxy;
	};
}}