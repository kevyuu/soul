#pragma once

#include "core/type.h"
#include "core/type_traits.h"
#include "runtime/data.h"

#define SOUL_ASSERT_MAIN_THREAD() SOUL_ASSERT(0, soul::runtime::System::Get().getThreadID() == 0, "This method is not thread safe. Please only call it only from main thread!")

namespace soul::runtime
{

	class System {
	public:

		System() = default;
		System(const System&) = delete;
		System& operator=(const System&) = delete;
		System(System&&) = delete;
		System& operator=(System&&) = delete;
		~System() = default;

		/*
			Initialize the Runtime System. Only call this from the main thread.
		*/
		void init(const Config& config);

		/*
			Cleanup all runtime system resource. Only call this from the main thread.
		*/
		void shutdown();

		void beginFrame(); 

		template<execution EXECUTE>
		TaskID taskCreate(TaskID parent, EXECUTE&& lambda) {
			static_assert(sizeof lambda <= sizeof(Task::storage),
				"Lambda size is too big."
				"Consider increase the storage size of the"
				"task or dynamically allocate the memory.");

			static auto call = [](TaskID taskID, void* data) {
				EXECUTE& lambda = *((EXECUTE*)data);
				lambda(taskID);
				lambda.~EXECUTE();
			};

			TaskID taskID = _taskCreate(parent, call);
			Task* task = _taskPtr(taskID);
			new(task->storage) EXECUTE(std::forward<EXECUTE>(lambda));
			return taskID;
		}

		template <typename FUNC>
		TaskID _parallelForTaskCreateRecursive(TaskID parent, uint32 start, uint32 dataCount, uint32 blockSize, FUNC&& func) {
			using TaskData = ParallelForTaskData<FUNC>;

			static_assert(sizeof(TaskData) <= sizeof(Task::storage),
				"ParallelForTaskData size is too big. TaskData = %d"
				"Consider to increase the storage size of the task.");

			static auto parallelFunc = [](TaskID taskID, void* data) {
				TaskData& taskData = (*(TaskData*)data);
				if (taskData.count > taskData.minCount) {
					uint32 leftCount = taskData.count / 2;
					TaskID leftTaskID = Get()._parallelForTaskCreateRecursive(taskID, taskData.start, leftCount, taskData.minCount, taskData.func);
					Get().taskRun(leftTaskID);

					uint32 rightCount = taskData.count - leftCount;
					TaskID rightTaskID = Get()._parallelForTaskCreateRecursive(taskID, taskData.start + leftCount, rightCount, taskData.minCount, taskData.func);
					Get().taskRun(rightTaskID);
				}
				else {
					for (soul_size i = 0; i < taskData.count; i++) {
						taskData.func(taskData.start + i);
					}
				}
			};

			TaskID taskID = _taskCreate(parent, std::move(parallelFunc));
			Task* task = _taskPtr(taskID);
			new(task->storage) TaskData(start, dataCount, blockSize, std::forward<FUNC>(func));
			return taskID;
		}

		template<typename FUNC> requires is_lambda_v<FUNC, void(int)>
		TaskID parallelForTaskCreate(TaskID parent, uint32 count, uint32 blockSize, FUNC&& func) {
			return _parallelForTaskCreateRecursive(parent, 0, count, blockSize, std::forward<FUNC>(func));
		}

		void taskWait(TaskID taskID);
		void taskRun(TaskID taskID);

		uint16 getThreadCount() const;
		uint16 getThreadID() const;
		ThreadContext& _threadContext();

		static System& Get() {
			[[clang::no_destroy]] static System instance;
			return instance;
		}

		void pushAllocator(memory::Allocator* allocator);
		void popAllocator();
		memory::Allocator* getContextAllocator();
		void* allocate(uint32 size, uint32 alignment);
		void deallocate(void* addr, uint32 size);
		TempAllocator* getTempAllocator();

	private:

		TaskID _taskCreate(TaskID parent, TaskFunc func);
		Task* _taskPtr(TaskID taskID);
		void _taskFinish(Task* task);
		static bool _taskIsComplete(Task* task);

		void _loop(ThreadContext* threadContext);
		void _execute(TaskID task);

		void _terminate();

		void _initRootTask();

		Database _db;
	};
}
