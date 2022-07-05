#pragma once

#include <mutex>

#include "memory/allocator.h"
#include "memory/util.h"

namespace soul::memory {

	struct AllocateParam {
		soul_size size = 0;
		soul_size alignment = 0;
		const char* tag = nullptr;
	};

	struct DeallocateParam {
		void* addr = nullptr;
		soul_size size = 0;
	};

	class Proxy {
	public:

		Proxy() = default;
		Proxy(const Proxy& other) = default;
		Proxy& operator=(const Proxy& other) = default;
		Proxy(Proxy&& other) = default;
		Proxy& operator=(Proxy&& other) = default;
		virtual ~Proxy() = default;

		virtual void* get_base_addr(void* addr) const { return addr; }
		[[nodiscard]] virtual soul_size get_base_size(const soul_size size) const { return size; }

		virtual void on_pre_init(const char* name) = 0;
		virtual void on_post_init() = 0;

		virtual AllocateParam on_pre_allocate(const AllocateParam& alloc_param) = 0;
		virtual Allocation on_post_allocate(Allocation allocation) = 0;

		virtual DeallocateParam on_pre_deallocate(const DeallocateParam& dealloc_param) = 0;
		virtual void on_post_deallocate() = 0;

		virtual void on_pre_cleanup() = 0;
		virtual void on_post_cleanup() = 0;
	};

	class NoOpProxy final : public Proxy {
	public:

		struct Config{};

		explicit NoOpProxy(const Config& config) {}

		void on_pre_init(const char* name) override {}
		void on_post_init() override {}

		AllocateParam on_pre_allocate(const AllocateParam& alloc_param) override { return alloc_param; }
		Allocation on_post_allocate(const Allocation allocation) override { return allocation; }

		DeallocateParam on_pre_deallocate(const DeallocateParam& dealloc_param) override { return dealloc_param; }
		void on_post_deallocate() override {}

		void on_pre_cleanup() override {}
		void on_post_cleanup() override {}
	};

	template <
		typename Proxy1 = NoOpProxy,
		typename Proxy2 = NoOpProxy,
		typename Proxy3 = NoOpProxy,
		typename Proxy4 = NoOpProxy,
		typename Proxy5 = NoOpProxy>
	class MultiProxy final : public Proxy {
	private:
		Proxy1 proxy1_;
		Proxy2 proxy2_;
		Proxy3 proxy3_;
		Proxy4 proxy4_;
		Proxy5 proxy5_;

	public:

		struct Config
		{
			typename Proxy1::Config config1;
			typename Proxy2::Config config2;
			typename Proxy3::Config config3;
			typename Proxy4::Config config4;
			typename Proxy5::Config config5;
			
			explicit Config(
				typename Proxy1::Config config1,
				typename Proxy2::Config config2 = NoOpProxy::Config(),
				typename Proxy3::Config config3 = NoOpProxy::Config(),
				typename Proxy4::Config config4 = NoOpProxy::Config(),
				typename Proxy5::Config config5 = NoOpProxy::Config()
			) : config1(config1), config2(config2), config3(config3), config4(config4), config5(config5) {}
		};

		explicit MultiProxy(const Config& config) :
			proxy1_(config.config1),
			proxy2_(config.config2),
			proxy3_(config.config3),
			proxy4_(config.config4),
			proxy5_(config.config5)
		{}

		void* get_base_addr(void* addr) const override
		{
			void* base_addr = proxy1_.get_base_addr(addr);
			base_addr = proxy2_.get_base_addr(base_addr);
			base_addr = proxy3_.get_base_addr(base_addr);
			base_addr = proxy4_.get_base_addr(base_addr);
			base_addr = proxy5_.get_base_addr(base_addr);
			return base_addr;
		}

		[[nodiscard]] soul_size get_base_size(const soul_size size) const override
		{
			soul_size base_size = proxy1_.get_base_size(size);
			base_size = proxy2_.get_base_size(base_size);
			base_size = proxy3_.get_base_size(base_size);
			base_size = proxy4_.get_base_size(base_size);
			base_size = proxy5_.get_base_size(base_size);
			return base_size;
		}

		void on_pre_init(const char* name) override {
			proxy1_.on_pre_init(name);
			proxy2_.on_pre_init(name);
			proxy3_.on_pre_init(name);
			proxy4_.on_pre_init(name);
			proxy5_.on_pre_init(name);
		}

		void on_post_init() override {
			proxy5_.on_post_init();
			proxy4_.on_post_init();
			proxy3_.on_post_init();
			proxy2_.on_post_init();
			proxy1_.on_post_init();
		}

		AllocateParam on_pre_allocate(const AllocateParam& alloc_param) override
		{
			AllocateParam param = alloc_param;
			param = proxy1_.on_pre_allocate(param);
			param = proxy2_.on_pre_allocate(param);
			param = proxy3_.on_pre_allocate(param);
			param = proxy4_.on_pre_allocate(param);
			param = proxy5_.on_pre_allocate(param);
			return param;
		}

		Allocation on_post_allocate(Allocation allocation) override
		{
			allocation = proxy5_.on_post_allocate(allocation);
			allocation = proxy4_.on_post_allocate(allocation);
			allocation = proxy3_.on_post_allocate(allocation);
			allocation = proxy2_.on_post_allocate(allocation);
			allocation = proxy1_.on_post_allocate(allocation);
			return allocation;
		}

		DeallocateParam on_pre_deallocate(const DeallocateParam& dealloc_param) override
		{
			DeallocateParam param = dealloc_param;
			param = proxy1_.on_pre_deallocate(param);
			param = proxy2_.on_pre_deallocate(param);
			param = proxy3_.on_pre_deallocate(param);
			param = proxy4_.on_pre_deallocate(param);
			param = proxy5_.on_pre_deallocate(param);
			return param;
		}

		void on_post_deallocate() override
		{
			proxy5_.on_post_deallocate();
			proxy4_.on_post_deallocate();
			proxy3_.on_post_deallocate();
			proxy2_.on_post_deallocate();
			proxy1_.on_post_deallocate();
		}

		void on_pre_cleanup() override
		{
			proxy1_.on_pre_cleanup();
			proxy2_.on_pre_cleanup();
			proxy3_.on_pre_cleanup();
			proxy4_.on_pre_cleanup();
			proxy5_.on_pre_cleanup();
		}

		void on_post_cleanup() override
		{
			proxy5_.on_post_cleanup();
			proxy4_.on_post_cleanup();
			proxy3_.on_post_cleanup();
			proxy2_.on_post_cleanup();
			proxy1_.on_post_cleanup();
		}
	};

	class CounterProxy final: public Proxy {

	public:
		struct Config {};

		explicit CounterProxy(const Config& config) {}

		CounterProxy() = default;
		void on_pre_init(const char* name) override { _counter = 0; }
		void on_post_init() override {}

		AllocateParam on_pre_allocate(const AllocateParam& alloc_param) override { _counter++; return alloc_param; }
		Allocation on_post_allocate(const Allocation allocation) override { return allocation; }

		DeallocateParam on_pre_deallocate(const DeallocateParam& dealloc_param) override { _counter--; return dealloc_param; }
		void on_post_deallocate() override {}

		void on_pre_cleanup() override { SOUL_ASSERT(0, _counter == 0, ""); }
		void on_post_cleanup() override {}

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
			: on_alloc_clear_value_(config.allocateClearValue), on_dealloc_clear_value_(config.freeClearValue) {}

		void on_pre_init(const char* name) override {}
		void on_post_init() override {}

		AllocateParam on_pre_allocate(const AllocateParam& alloc_param) override {
			current_alloc_size_ = alloc_param.size;
			return alloc_param;
		}
		Allocation on_post_allocate(const Allocation allocation) override {
			SOUL_ASSERT(0, allocation.size == current_alloc_size_, "");
			memset(allocation.addr, on_alloc_clear_value_, current_alloc_size_);
			return allocation;
		}

		DeallocateParam on_pre_deallocate(const DeallocateParam& dealloc_param) override {
			if (dealloc_param.addr != nullptr)
			{
				SOUL_ASSERT(0, dealloc_param.size != 0, "This proxy need size in its deallocate call");
				memset(dealloc_param.addr, on_dealloc_clear_value_, dealloc_param.size);
			}
			return dealloc_param;
		}
		void on_post_deallocate() override {}

		void on_pre_cleanup() override {}
		void on_post_cleanup() override {}

	private:
		const char on_alloc_clear_value_;
		const char on_dealloc_clear_value_;
		
		soul_size current_alloc_size_ = 0;
	};

	class BoundGuardProxy final: public Proxy {

	public:

		struct Config{};

		explicit BoundGuardProxy(const Config& config) {}

		void* get_base_addr(void* addr) const override { return util::pointer_sub(addr, GUARD_SIZE); }
		[[nodiscard]] soul_size get_base_size(const soul_size size) const override { return size - 2llu * GUARD_SIZE; }

		void on_pre_init(const char* name) override {}
		void on_post_init() override {}

		AllocateParam on_pre_allocate(const AllocateParam& alloc_param) override {
			current_alloc_size_ = alloc_param.size;
			return {alloc_param.size + 2llu * GUARD_SIZE, GUARD_SIZE, alloc_param.tag};
		}

		Allocation on_post_allocate(const Allocation allocation) override {
			memset(allocation.addr, GUARD_FLAG, GUARD_SIZE);
			memset(util::pointer_add(allocation.addr, GUARD_SIZE + current_alloc_size_), GUARD_FLAG, GUARD_SIZE);
			SOUL_ASSERT(0, allocation.size > 2llu * GUARD_SIZE, "");
			return {util::pointer_add(allocation.addr, GUARD_SIZE), allocation.size - 2llu * GUARD_SIZE};
		}

		DeallocateParam on_pre_deallocate(const DeallocateParam& dealloc_param) override {

			if (dealloc_param.addr != nullptr)
			{
				SOUL_ASSERT(0, dealloc_param.size != 0, "This proxy need size in its deallocate call");
				auto base_front = soul::cast<byte*>(util::pointer_sub(dealloc_param.addr, GUARD_SIZE));
				auto base_back = soul::cast<byte*>(util::pointer_add(dealloc_param.addr, dealloc_param.size));
				for (soul_size i = 0; i < GUARD_SIZE; i++) {
					SOUL_ASSERT(0, base_front[i] == GUARD_FLAG, "");
					SOUL_ASSERT(0, base_back[i] == GUARD_FLAG, "");
				}
				return {util::pointer_sub(dealloc_param.addr, GUARD_SIZE), dealloc_param.size + 2llu * GUARD_SIZE};
			}
			return dealloc_param;
			
		}
		void on_post_deallocate() override {}

		void on_pre_cleanup() override {}
		void on_post_cleanup() override {}

	private:
		static constexpr uint32 GUARD_SIZE = alignof(std::max_align_t);
		static constexpr byte GUARD_FLAG = 0xAA;
		soul_size current_alloc_size_ = 0;

	};

	class ProfileProxy final: public Proxy {

	public:

		struct Config{};
		explicit ProfileProxy(const Config& config) {}

		void on_pre_init(const char* name) override;
		void on_post_init() override {}

		AllocateParam on_pre_allocate(const AllocateParam& alloc_param) override;
		Allocation on_post_allocate(Allocation allocation) override;

		DeallocateParam on_pre_deallocate(const DeallocateParam& dealloc_param) override;
		void on_post_deallocate() override {}

		void on_pre_cleanup() override;
		void on_post_cleanup() override {}

	private:
		const char* name_ = nullptr;
		AllocateParam current_alloc_;

	};

	class MutexProxy final: public Proxy {
	public:
		struct Config{};
		explicit MutexProxy(const Config& config) {}

		void on_pre_init(const char* name) override{}
		void on_post_init() override{}

		AllocateParam on_pre_allocate(const AllocateParam& alloc_param) override { mutex_.lock(); return alloc_param; }
	    Allocation on_post_allocate(const Allocation allocation) override { mutex_.unlock(); return allocation; }

		DeallocateParam on_pre_deallocate(const DeallocateParam& dealloc_param) override { mutex_.lock(); return dealloc_param; }
	    void on_post_deallocate() override { mutex_.unlock(); }

		void on_pre_cleanup() override {}
		void on_post_cleanup() override {}

	private:
		std::mutex mutex_;
	};

	template<typename BackingAllocator,
	         typename Proxy = NoOpProxy>
	class ProxyAllocator final: public Allocator {

	public:

		BackingAllocator* allocator;

		ProxyAllocator() = delete;
		explicit ProxyAllocator(BackingAllocator* allocator, typename Proxy::Config proxyConfig) :
			Allocator(allocator->name()),
			allocator(allocator),
			proxy_(proxyConfig) {
			proxy_.on_pre_init(name());
			proxy_.on_post_init();
		}
		template <typename Cproxy>
		explicit ProxyAllocator(const char* name, BackingAllocator* allocator, Cproxy&& proxy) :
			Allocator(name),
			allocator(allocator),
			proxy_(std::forward<Cproxy>(proxy)) {
			proxy_.on_pre_init(name);
			proxy_.on_post_init();
		}
		ProxyAllocator(const ProxyAllocator& other) = delete;
		ProxyAllocator& operator=(const ProxyAllocator& other) = delete;
		ProxyAllocator(ProxyAllocator&& other) = delete;
		ProxyAllocator& operator=(ProxyAllocator&& other) = delete;
		~ProxyAllocator() override = default;
		
		void reset() override {
			proxy_.on_pre_cleanup();
			allocator->reset();
			proxy_.on_post_cleanup();
		}

		Allocation try_allocate(const soul_size size, const soul_size alignment, const char* tag) override {
			if (size == 0) return { nullptr, 0 };
			AllocateParam param = { size, alignment, tag };
			param = proxy_.on_pre_allocate(param);
			Allocation allocation = allocator->try_allocate(param.size, param.alignment, name());
			return proxy_.on_post_allocate(allocation);
		}

		[[nodiscard]] soul_size get_allocation_size(void* addr) const override
		{
			void* base_addr = proxy_.get_base_addr(addr);
			return proxy_.get_base_size(allocator->get_allocation_size(base_addr));
		}

		void deallocate(void* addr) override {
			if (addr == nullptr) return;
			DeallocateParam param = { addr, get_allocation_size(addr) };
			param = proxy_.on_pre_deallocate(param);
			allocator->deallocate(param.addr);
			proxy_.on_post_deallocate();
		}

		[[nodiscard]] void* get_marker() const noexcept
		{
			return allocator->get_marker();
		}

		void rewind(void* addr) noexcept
		{
			allocator->rewind(addr);
		}

	private:
		Proxy proxy_;
	};
}
