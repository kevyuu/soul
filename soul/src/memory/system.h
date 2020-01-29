#pragma once

namespace Soul { namespace Memory {

	class System {
	public:

		struct Config {
			Allocator* const defaultAllocator;
		};

		void init(Config config) {
			_defaultAllocator = config.defaultAllocator;
		}

		inline static System& Get() {
			static System instance;
			return instance;
		}

		Allocator* const getDefaultAllocator() {
			SOUL_ASSERT(0, _defaultAllocator != nullptr, "Memory system have not been initialized!");
			return _defaultAllocator;
		}

	private:

		Allocator* _defaultAllocator = nullptr;
		System() = default;

	};

}}