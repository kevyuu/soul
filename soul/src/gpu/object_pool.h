#pragma once

#include "memory/allocator.h"
#include "core/array.h"
#include "core/config.h"

#include "core/mutex.h"

namespace soul::gpu
{

	template <typename T, is_lockable_v Lockable = Mutex, size_t BLOCK_SIZE = 512>
	class ConcurrentObjectPool
	{
	public:
		using ID = T*;
		static constexpr T* const NULLVAL = nullptr;

		explicit ConcurrentObjectPool(soul::memory::Allocator* allocator = GetDefaultAllocator()) noexcept : allocator_(allocator)
		{}
		ConcurrentObjectPool(const ConcurrentObjectPool&) = delete;
		ConcurrentObjectPool& operator=(const ConcurrentObjectPool& rhs) = delete;
		ConcurrentObjectPool(ConcurrentObjectPool&&) = delete;
		ConcurrentObjectPool& operator=(ConcurrentObjectPool&& rhs) = delete;
		~ConcurrentObjectPool()
		{
			soul_size num_objects = BLOCK_SIZE / sizeof(T);
			for (T* memory : memories_)
			{
				allocator_->deallocate(memory, num_objects * sizeof(T));
			}
		}

		template <typename... ARGS>
		ID create(ARGS&&... args)
		{
			std::lock_guard guard(mutex_);
			if (vacants_.empty())
			{
				soul_size num_objects = BLOCK_SIZE / sizeof(T);
				T* memory = static_cast<T*>(allocator_->allocate(num_objects * sizeof(T), alignof(T), ""));
				if (!memory)
				{
					return nullptr;
				}
				for (soul_size object_idx = 0; object_idx < num_objects; object_idx++)
				{
					vacants_.push_back(&memory[object_idx]);
				}
				memories_.push_back(memory);
			}
			T* ptr = vacants_.back();
			vacants_.pop();
			new(ptr) T(std::forward<ARGS>(args)...);
			return ptr;
		}

		void destroy(ID id)
		{
			id->~T();
			std::lock_guard guard(mutex_);
			vacants_.push_back(id);
		}

		T* get(ID id) const
		{
			return id;
		}

	private:
		mutable Lockable mutex_;
		Array<T*> vacants_;
		Array<T*> memories_;
		memory::Allocator* allocator_;
	};

	template<typename T, size_t BLOCK_SIZE = 512>
	using ObjectPool = ConcurrentObjectPool<T, NullMutex, BLOCK_SIZE>;
}