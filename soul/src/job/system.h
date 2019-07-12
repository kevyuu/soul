#pragma once
#include "core/type.h"
#include "job/data.h"

namespace Soul {
	namespace Job {
		
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
			TaskID taskCreate(TaskID parent, T lambda);

			template <typename T>
			TaskID parallelForTaskCreate(TaskID parent, T* data, uint32 dataCount, uint32 blockSize, void(*func)(T* data, uint32 count));

			void taskWait(TaskID taskID);
			void taskRun(TaskID taskID);

			inline static System& Get() {
				static System instance;
				return instance;
			}

			TaskID _taskCreate(TaskID parent, TaskFunc func);
			Task* _getTaskPtr(TaskID taskID);
			void _taskFinish(Task* task);
			void _loop(ThreadState* threadState);
			void _execute(TaskID task);
			bool _isTaskComplete(Task* task);
			void _terminate();

			Database _db;
		};

		template<typename T>
		TaskID System::taskCreate(TaskID parent, T lambda) {
			
			static_assert(sizeof lambda <= sizeof(Task::storage), 
				"Lambda size is too big."
				"Consider increase the storage size of the" 
				"task or dyanmically allocate the memory.");

			static auto call = [](TaskID taskID, void* data) {
				T& lambda = *((T*)data);
				lambda(taskID);
				lambda.~T();
			};

			TaskID taskID = _taskCreate(parent, call);
			Task* task = _getTaskPtr(taskID);
			new(task->storage) T(std::move(lambda));
			return taskID;

		}

		template<typename T>
		TaskID System::parallelForTaskCreate(TaskID parent, T* data, uint32 dataCount, uint32 blockSize, void(*func)(T* data, uint32 count)) {
			
			using TaskData = ParallelForTaskData<T>;

			static_assert(sizeof(TaskData) <= sizeof(Task::storage),
				"ParallelForTaskData size is too big."
				"Consider increase the storage size of the task.");

			static auto parallelFunc = [](TaskID taskID, void* data) {
				TaskData& taskData = (*(TaskData*)data);
				if (taskData.count > taskData.minCount) {
					uint32 leftCount = taskData.count / 2;
					TaskID leftTaskID = Get().parallelForTaskCreate(taskID, taskData.data, leftCount, taskData.minCount, taskData.func);
					Get().taskRun(leftTaskID);

					uint32 rightCount = taskData.count - leftCount;
					TaskID rightTaskID = Get().parallelForTaskCreate(taskID, taskData.data + leftCount, rightCount, taskData.minCount, taskData.func);
					Get().taskRun(rightTaskID);
				}
				else {
					taskData.func(taskData.data, taskData.count);
				}
			};
			
			TaskID taskID = _taskCreate(parent, parallelFunc);
			Task* task = _getTaskPtr(taskID);
			new(task->storage) TaskData(data, dataCount, blockSize, func);
			return taskID;

		}

	}
}