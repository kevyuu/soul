#pragma once
#include "core/type.h"
#include "core/util.h"
#include "runtime/data.h"

#define SOUL_ASSERT_MAIN_THREAD() SOUL_ASSERT(0, Runtime::System::Get().getThreadID() == 0, "This method is not thread safe. Please only call it only from main thread!")

namespace Soul { namespace Runtime {

	class System {
	public:

		/*
			Initialize the Runtime System. Only call this from the main thread.
		*/
		void init(const Config& config);

		/*
			Cleanup all runtime system resource. Only call this from the main thread.
		*/
		void shutdown();

		void beginFrame(); 

		template <SOUL_TEMPLATE_ARG_LAMBDA(Execute, void(TaskID))>
		TaskID taskCreate(TaskID parent, Execute&& lambda) {
			static_assert(sizeof lambda <= sizeof(Task::storage),
						  "Lambda size is too big."
						  "Consider increase the storage size of the"
						  "task or dynamically allocate the memory.");

			static auto call = [](TaskID taskID, void* data) {
				Execute& lambda = *((Execute*)data);
				lambda(taskID);
				lambda.~Execute();
			};

			TaskID taskID = _taskCreate(parent, call);
			Task* task = _taskPtr(taskID);
			new(task->storage) Execute(std::forward<Execute>(lambda));
			return taskID;
		}

		template <typename Func>
		TaskID _parallelForTaskCreateRecursive(TaskID parent, uint32 start, uint32 dataCount, uint32 blockSize, Func&& func) {
			using TaskData = ParallelForTaskData<Func>;

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
					for (int i = 0; i < taskData.count; i++) {
						taskData.func(taskData.start + i);
					}
				}
			};

			TaskID taskID = _taskCreate(parent, std::move(parallelFunc));
			Task* task = _taskPtr(taskID);
			new(task->storage) TaskData(start, dataCount, blockSize, std::forward<Func>(func));
			return taskID;
		}

		template <SOUL_TEMPLATE_ARG_LAMBDA(Func, void(int))>
		inline TaskID parallelForTaskCreate(TaskID parent, uint32 count, uint32 blockSize, Func&& func) {
			return _parallelForTaskCreateRecursive(parent, 0, count, blockSize, std::forward<Func>(func));
		}

		void taskWait(TaskID taskID);
		void taskRun(TaskID taskID);

		uint16 getThreadCount() const;
		uint16 getThreadID() const;
		_ThreadContext& _threadContext();

		inline static System& Get() {
			static System instance;
			return instance;
		}

		void pushAllocator(Memory::Allocator* allocator);
		void popAllocator();
		Memory::Allocator* getContextAllocator();
		void* allocate(uint32 size, uint32 alignment);
		void deallocate(void* addr, uint32 size);
		TempAllocator* getTempAllocator();

	private:

		TaskID _taskCreate(TaskID parent, TaskFunc func);
		Task* _taskPtr(TaskID taskID);
		void _taskFinish(Task* task);
		bool _taskIsComplete(Task* task) const;

		void _loop(_ThreadContext* threadContext);
		void _execute(TaskID task);

		void _terminate();

		void _initSentinel();

		Database _db;
	};
}}