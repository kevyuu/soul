#pragma once
#include "core/type.h"
#include "job/data.h"
#include <utility>

namespace Soul { namespace Job {
		
	struct System {

		struct Config {
			uint16 threadCount; // 0 to use hardware thread count
			uint16 taskPoolCount;
		};
		
		/*
			Initialize the Job System. Only call this from the main thread.
		*/
		void init(const Config& config);

		/*
			Cleanup all job system resource. Only call this from the main thread.
		*/
		void shutdown();

		void beginFrame(); 

		template <typename T>
		TaskID taskCreate(TaskID parent, T&& lambda);

		template <typename Func>
		TaskID _parallelForTaskCreateRecursive(TaskID parent, uint32 start, uint32 count, uint32 blockSize, Func&& func);

		template <typename Func>
		inline TaskID parallelForTaskCreate(TaskID parent, uint32 count, uint32 blockSize, Func&& func) {
			return _parallelForTaskCreateRecursive(parent, 0, count, blockSize, std::forward<Func>(func));
		}

		void taskWait(TaskID taskID);
		void taskRun(TaskID taskID);

		uint16 getThreadCount() const;
		uint16 getThreadID() const;

		inline static System& Get() {
			static System instance;
			return instance;
		}

		TaskID _taskCreate(TaskID parent, TaskFunc func);
		Task* _getTaskPtr(TaskID taskID);
		void _taskFinish(Task* task);
		void _loop(ThreadState* threadState);
		void _execute(TaskID task);
		bool _isTaskComplete(Task* task) const;
		void _terminate();

		Database _db;
	};

	template<typename T>
	TaskID System::taskCreate(TaskID parent, T&& lambda) {
		
		static_assert(sizeof lambda <= sizeof(Task::storage), 
			"Lambda size is too big."
			"Consider increase the storage size of the" 
			"task or dynamically allocate the memory.");

		static auto call = [](TaskID taskID, void* data) {
			T& lambda = *((T*)data);
			lambda(taskID);
			lambda.~T();
		};

		TaskID taskID = _taskCreate(parent, call);
		Task* task = _getTaskPtr(taskID);
		new(task->storage) T(std::forward<T>(lambda));
		return taskID;

	}

	template<typename Func>
	TaskID System::_parallelForTaskCreateRecursive(TaskID parent, uint32 start, uint32 dataCount, uint32 blockSize, Func&& func) {
		
		using TaskData = ParallelForTaskData<Func>;

		static_assert(sizeof(TaskData) <= sizeof(Task::storage),
			"ParallelForTaskData size is too big."
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
		Task* task = _getTaskPtr(taskID);
		new(task->storage) TaskData(start, dataCount, blockSize, std::forward<Func>(func));
		return taskID;
	}
}}