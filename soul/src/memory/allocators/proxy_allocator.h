#pragma once

#include <mutex>

#include "core/compiler.h"

#include "memory/allocator.h"
#include "memory/util.h"

namespace Soul::Memory {

	struct AllocateParam {
		soul_size size = 0;
		soul_size alignment = 0;
		const char* tag = nullptr;

		AllocateParam() = default;
		AllocateParam(soul_size size, soul_size alignment, const char* tag) noexcept : size(size), alignment(alignment), tag(tag) {}
	};

	struct DeallocateParam {
		void* addr = nullptr;
		soul_size size = 0;

		DeallocateParam() = default;
		DeallocateParam(void* addr, soul_size size) noexcept : addr(addr), size(size) {}
	};

	class Proxy {
	public:

		Proxy() = default;
		Proxy(const Proxy& other) = default;
		Proxy& operator=(const Proxy& other) = default;
		Proxy(Proxy&& other) = default;
		Proxy& operator=(Proxy&& other) = default;
		virtual ~Proxy() = default;

		virtual void onPreInit(const char* name) = 0;
		virtual void onPostInit() = 0;

		virtual AllocateParam onPreAllocate(const AllocateParam& allocParam) = 0;
		virtual Allocation onPostAllocate(Allocation allocation) = 0;

		virtual DeallocateParam onPreDeallocate(const DeallocateParam& deallocParam) = 0;
		virtual void onPostDeallocate() = 0;

		virtual void onPreCleanup() = 0;
		virtual void onPostCleanup() = 0;
	};

	class NoOpProxy final : public Proxy {
	public:

		struct Config{};

		explicit NoOpProxy(const Config& config) {}

		void onPreInit(const char* name) override {}
		void onPostInit() override {}

		AllocateParam onPreAllocate(const AllocateParam& allocParam) override { return allocParam; }
		Allocation onPostAllocate(Allocation allocation) override { return allocation; }

		DeallocateParam onPreDeallocate(const DeallocateParam& deallocParam) override { return deallocParam; }
		void onPostDeallocate() override {}

		void onPreCleanup() override {}
		void onPostCleanup() override {}
	};

	template <
		typename PROXY1 = NoOpProxy,
		typename PROXY2 = NoOpProxy,
		typename PROXY3 = NoOpProxy,
		typename PROXY4 = NoOpProxy,
		typename PROXY5 = NoOpProxy>
	class MultiProxy final : public Proxy {
	private:
		PROXY1 _proxy1;
		PROXY2 _proxy2;
		PROXY3 _proxy3;
		PROXY4 _proxy4;
		PROXY5 _proxy5;

	public:

		struct Config
		{
			typename PROXY1::Config config1;
			typename PROXY2::Config config2;
			typename PROXY3::Config config3;
			typename PROXY4::Config config4;
			typename PROXY5::Config config5;
			
			explicit Config(
				typename PROXY1::Config config1,
				typename PROXY2::Config config2 = NoOpProxy::Config(),
				typename PROXY3::Config config3 = NoOpProxy::Config(),
				typename PROXY4::Config config4 = NoOpProxy::Config(),
				typename PROXY5::Config config5 = NoOpProxy::Config()
			) : config1(config1), config2(config2), config3(config3), config4(config4), config5(config5) {}
		};

		explicit MultiProxy(const Config& config) :
			_proxy1(config.config1),
			_proxy2(config.config2),
			_proxy3(config.config3),
			_proxy4(config.config4),
			_proxy5(config.config5)
		{}

		void onPreInit(const char* name) override {
			_proxy1.onPreInit(name);
			_proxy2.onPreInit(name);
			_proxy3.onPreInit(name);
			_proxy4.onPreInit(name);
			_proxy5.onPreInit(name);
		}

		void onPostInit() override {
			_proxy5.onPostInit();
			_proxy4.onPostInit();
			_proxy3.onPostInit();
			_proxy2.onPostInit();
			_proxy1.onPostInit();
		}

		AllocateParam onPreAllocate(const AllocateParam& allocParam) override
		{
			AllocateParam param = allocParam;
			param = _proxy1.onPreAllocate(param);
			param = _proxy2.onPreAllocate(param);
			param = _proxy3.onPreAllocate(param);
			param = _proxy4.onPreAllocate(param);
			param = _proxy5.onPreAllocate(param);
			return param;
		}

		Allocation onPostAllocate(Allocation allocation) override
		{
			allocation = _proxy5.onPostAllocate(allocation);
			allocation = _proxy4.onPostAllocate(allocation);
			allocation = _proxy3.onPostAllocate(allocation);
			allocation = _proxy2.onPostAllocate(allocation);
			allocation = _proxy1.onPostAllocate(allocation);
			return allocation;
		}

		DeallocateParam onPreDeallocate(const DeallocateParam& deallocParam) override
		{
			DeallocateParam param = deallocParam;
			param = _proxy1.onPreDeallocate(param);
			param = _proxy2.onPreDeallocate(param);
			param = _proxy3.onPreDeallocate(param);
			param = _proxy4.onPreDeallocate(param);
			param = _proxy5.onPreDeallocate(param);
			return param;
		}

		void onPostDeallocate() final
		{
			_proxy5.onPostDeallocate();
			_proxy4.onPostDeallocate();
			_proxy3.onPostDeallocate();
			_proxy2.onPostDeallocate();
			_proxy1.onPostDeallocate();
		}

		void onPreCleanup() override
		{
			_proxy1.onPreCleanup();
			_proxy2.onPreCleanup();
			_proxy3.onPreCleanup();
			_proxy4.onPreCleanup();
			_proxy5.onPreCleanup();
		}

		void onPostCleanup() override
		{
			_proxy5.onPostCleanup();
			_proxy4.onPostCleanup();
			_proxy3.onPostCleanup();
			_proxy2.onPostCleanup();
			_proxy1.onPostCleanup();
		}
	};

	class CounterProxy final: public Proxy {

	public:
		struct Config {};

		explicit CounterProxy(const Config& config) {}

		CounterProxy() = default;
		void onPreInit(const char* name) override { _counter = 0; }
		void onPostInit() override {}

		AllocateParam onPreAllocate(const AllocateParam& allocParam) override { _counter++; return allocParam; }
		Allocation onPostAllocate(Allocation allocation) override { return allocation; }

		DeallocateParam onPreDeallocate(const DeallocateParam& deallocParam) override { _counter--; return deallocParam; }
		void onPostDeallocate() override {}

		void onPreCleanup() override { SOUL_ASSERT(0, _counter == 0, ""); }
		void onPostCleanup() override {}

	private:
		soul_size _counter = 0;

	};

	class ClearValuesProxy final: public Proxy {

	public:

		struct Config
		{
			char allocateClearValue;
			char freeClearValue;
		};

		explicit ClearValuesProxy(const Config& config)
			: _onAllocClearValue(config.allocateClearValue), _onDeallocClearValue(config.freeClearValue) {}

		void onPreInit(const char* name) override {}
		void onPostInit() override {}

		AllocateParam onPreAllocate(const AllocateParam& allocParam) override {
			_currentAllocSize = allocParam.size;
			return allocParam;
		}
		Allocation onPostAllocate(Allocation allocation) override {
			SOUL_ASSERT(0, allocation.size == _currentAllocSize, "");
			memset(allocation.addr, _onAllocClearValue, _currentAllocSize);
			return allocation;
		}

		DeallocateParam onPreDeallocate(const DeallocateParam& deallocParam) override {
			memset(deallocParam.addr, _onDeallocClearValue, deallocParam.size);
			return deallocParam;
		}
		void onPostDeallocate() override {}

		void onPreCleanup() override {}
		void onPostCleanup() override {}

	private:
		const char _onAllocClearValue;
		const char _onDeallocClearValue;
		
		soul_size _currentAllocSize = 0;
	};

	class BoundGuardProxy final: public Proxy {

	public:

		struct Config{};

		explicit BoundGuardProxy(const Config& config) {}

		void onPreInit(const char* name) final{}
		void onPostInit() final {}

		AllocateParam onPreAllocate(const AllocateParam& allocParam) final {
			_currentAllocSize = allocParam.size;
			return AllocateParam(allocParam.size + 2 * GUARD_SIZE, GUARD_SIZE, allocParam.tag);
		}

		Allocation onPostAllocate(Allocation allocation) final {
			memset(allocation.addr, GUARD_FLAG, GUARD_SIZE);
			memset(Util::Add(allocation.addr, GUARD_SIZE + _currentAllocSize), GUARD_FLAG, GUARD_SIZE);
			SOUL_ASSERT(0, allocation.size > 2 * GUARD_SIZE, "");
			return Allocation(Util::Add(allocation.addr, GUARD_SIZE), allocation.size - 2 * GUARD_SIZE);
		}

		DeallocateParam onPreDeallocate(const DeallocateParam& deallocParam) final {
			// TODO(kevinyu): Make faster
			auto baseFront = (byte*) Util::Sub(deallocParam.addr, GUARD_SIZE);
			auto baseBack = (byte*) Util::Add(deallocParam.addr, deallocParam.size);
			for (soul_size i = 0; i < GUARD_SIZE; i++) {
				SOUL_ASSERT(0, baseFront[i] == GUARD_FLAG, "");
				SOUL_ASSERT(0, baseBack[i] == GUARD_FLAG, "");
			}
			return DeallocateParam( Util::Sub(deallocParam.addr, GUARD_SIZE), deallocParam.size + 2 * GUARD_SIZE );
		}
		void onPostDeallocate() override {}

		void onPreCleanup() override {}
		void onPostCleanup() override {}

	private:
		static constexpr uint32 GUARD_SIZE = alignof(std::max_align_t);
		static constexpr byte GUARD_FLAG = 0xAA;
		soul_size _currentAllocSize = 0;

	};

	class ProfileProxy final: public Proxy {

	public:

		struct Config{};
		explicit ProfileProxy(const Config& config) {}

		void onPreInit(const char* name) override;
		void onPostInit() override {}

		AllocateParam onPreAllocate(const AllocateParam& allocParam) override;
		Allocation onPostAllocate(Allocation allocation) override;

		DeallocateParam onPreDeallocate(const DeallocateParam& deallocParam) override;
		void onPostDeallocate() override {}

		void onPreCleanup() override;
		void onPostCleanup() override {}

	private:
		const char* _name = nullptr;
		AllocateParam _currentAlloc;

	};

	class MutexProxy final: public Proxy {
	public:
		struct Config{};
		explicit MutexProxy(const Config& config) {}

		void onPreInit(const char* name) override{}
		void onPostInit() override{}

		AllocateParam onPreAllocate(const AllocateParam& allocParam) override { mutex.lock(); return allocParam; }
		Allocation onPostAllocate(Allocation allocation) override { mutex.unlock(); return allocation; }

		DeallocateParam onPreDeallocate(const DeallocateParam& deallocParam) override { mutex.lock(); return deallocParam; }
		void onPostDeallocate() override { mutex.unlock(); }

		void onPreCleanup() override {}
		void onPostCleanup() override {}

	private:
		std::mutex mutex;
	};

	template<typename BACKING_ALLOCATOR,
	         typename PROXY = NoOpProxy>
	class ProxyAllocator final: public Allocator {

	public:

		BACKING_ALLOCATOR* allocator;

		ProxyAllocator() = delete;
		explicit ProxyAllocator(BACKING_ALLOCATOR* allocator, typename PROXY::Config proxyConfig) :
			Allocator(allocator->name()),
			allocator(allocator),
			_proxy(proxyConfig) {
			_proxy.onPreInit(name());
			_proxy.onPostInit();
		}
		template <typename CPROXY>
		explicit ProxyAllocator(const char* name, BACKING_ALLOCATOR* allocator, CPROXY&& proxy) :
			Allocator(name),
			allocator(allocator),
			_proxy(std::forward<CPROXY>(proxy)) {
			_proxy.onPreInit(name);
			_proxy.onPostInit();
		}
		ProxyAllocator(const ProxyAllocator& other) = delete;
		ProxyAllocator& operator=(const ProxyAllocator& other) = delete;
		ProxyAllocator(ProxyAllocator&& other) = delete;
		ProxyAllocator& operator=(ProxyAllocator&& other) = delete;
		~ProxyAllocator() override = default;

		
		void reset() override {
			_proxy.onPreCleanup();
			allocator->reset();
			_proxy.onPostCleanup();
		}

		Allocation tryAllocate(soul_size size, soul_size alignment, const char* tag) override {
			if (size == 0) return { nullptr, 0 };
			AllocateParam param = { size, alignment, tag };
			param = _proxy.onPreAllocate(param);
			Allocation allocation = allocator->tryAllocate(param.size, param.alignment, name());
			return _proxy.onPostAllocate(allocation);
		}

		void deallocate(void* addr, soul_size size) override {
			if (addr == nullptr || size == 0) return;
			DeallocateParam param = { addr, size };
			param = _proxy.onPreDeallocate(param);
			allocator->deallocate(param.addr, size);
			_proxy.onPostDeallocate();
		}

		SOUL_NODISCARD void* getMarker() const noexcept
		{
			return allocator->getMarker();
		}

		void rewind(void* addr) noexcept
		{
			allocator->rewind(addr);
		}

	private:
		PROXY _proxy;
	};
}
