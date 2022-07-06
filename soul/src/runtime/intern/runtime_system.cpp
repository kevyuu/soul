#include "core/dev_util.h"
#include "core/architecture.h"
#include "runtime/system.h"

#include "memory/allocators/proxy_allocator.h"
#include "memory/allocators/linear_allocator.h"

#include <thread>

static unsigned long randXorshf96() {
	static thread_local unsigned long x = 123456789, y = 362436069, z = 521288629;
	unsigned long t;
	x ^= x << 16;
	x ^= x >> 5;
	x ^= x << 1;

	t = x;
	x = y;
	y = z;
	z = t ^ x ^ y;

	return z;
}

namespace soul::runtime {

	thread_local ThreadContext* Database::gThreadContext = nullptr;
	
	void System::_execute(TaskID taskID) {
		{
			std::lock_guard<std::mutex> lock(_db.loopMutex);
			_db.activeTaskCount -= 1;
		}
		Task* task = _taskPtr(taskID);
		task->func(taskID, task->storage);
		_taskFinish(task);
	}

	void System::_loop(ThreadContext* threadContext) {
		Database::gThreadContext = threadContext;

		char threadName[512];
		sprintf(threadName, "Worker Thread = %d", getThreadID());
		SOUL_PROFILE_THREAD_SET_NAME(threadName);

		char tempAllocatorName[512];
		memory::LinearAllocator linearAllocator(tempAllocatorName, 20 * ONE_MEGABYTE, getContextAllocator());
		TempAllocator tempAllocator(&linearAllocator, runtime::TempProxy::Config());
		threadContext->tempAllocator = &tempAllocator;

		while (true) {
			TaskID taskID = threadContext->taskDeque.pop();
			while (taskID == TaskID::NULLVAL()) {
				
				{
					std::unique_lock<std::mutex> lock(_db.loopMutex);
					while(_db.activeTaskCount == 0 && !_db.isTerminated.load(std::memory_order_relaxed)) {
						_db.loopCondVar.wait(lock);
					}
				}

				if (_db.isTerminated.load(std::memory_order_relaxed)) {
					break;
				}

				soul_size threadIndex = (randXorshf96() % _db.threadCount);
				taskID = _db.threadContexts[threadIndex].taskDeque.steal();
			}

			if (_db.isTerminated.load(std::memory_order_relaxed)) {  
				break;
			}
			_execute(taskID);
		}
	}

	void System::_terminate() {
		_db.isTerminated.store(true);
		{
			std::lock_guard<std::mutex> lock(_db.loopMutex);
		}
		_db.loopCondVar.notify_all();
	}

	void System::beginFrame() {
		SOUL_PROFILE_ZONE();
		SOUL_ASSERT_MAIN_THREAD();

		taskWait(TaskID::NULLVAL());

		_initRootTask();

		_db.threadContexts[0].tempAllocator->reset();

		for (soul_size i = 1; i < _db.threadCount; i++) {
			_db.threadContexts[i].taskCount = 0;
			_db.threadContexts[i].taskDeque.reset();
			_db.threadContexts[i].tempAllocator->reset();
		}
	}

	TaskID System::_taskCreate(TaskID parent, TaskFunc func) {
		ThreadContext* threadState = Database::gThreadContext;

		const uint16 threadIndex = threadState->threadIndex;
		const uint16 taskIndex = threadState->taskCount;
		const TaskID taskID = TaskID(threadIndex, taskIndex);

		threadState->taskCount += 1;
		Task& task = threadState->taskPool[taskIndex];
		task.parentID = parent;
		task.unfinishedCount.store(1, std::memory_order_relaxed);
		task.func = func;

		Task* parentTask = _taskPtr(parent);
		parentTask->unfinishedCount.fetch_add(1, std::memory_order_relaxed);

		return taskID;
	}

	Task* System::_taskPtr(TaskID taskID) {
		
		soul_size threadIndex = taskID.threadIndex();
		soul_size taskIndex = taskID.taskIndex();

		return &_db.threadContexts[threadIndex].taskPool[taskIndex];

	}

	bool System::_taskIsComplete(Task* task)
	{
		// NOTE(kevinyu): Synchronize with fetch_sub in _finishTask() to make sure the task is executed before we return true.
		return task->unfinishedCount.load(std::memory_order_acquire) == 0;
	}

	// TODO(kevinyu) : Implement stealing from other deque when waiting for task. 
	// Currently we only try to pop task from our thread local deque.
	void System::taskWait(TaskID taskID) {
		ThreadContext* threadState = Database::gThreadContext;
		Task* taskToWait = _taskPtr(taskID);
		while (!_taskIsComplete(taskToWait)) {

			TaskID taskToDo = threadState->taskDeque.pop();

			if (!taskToDo.isNull()) {
				_execute(taskToDo);
			}
			else {
				std::unique_lock<std::mutex> lock(_db.waitMutex);
				while (!_taskIsComplete(taskToWait)) {
					_db.waitCondVar.wait(lock);
				}
			}
		}
	}

	void System::_initRootTask() {
		// NOTE(kevinyu): TaskID 0 is ROOT. TaskID 0 is used as parent for all task
		_db.threadContexts[0].taskPool[0].unfinishedCount.store(0, std::memory_order_relaxed);
		_db.threadContexts[0].taskCount = 1;
		_db.threadContexts[0].taskDeque.reset();
	}

	void System::init(const Config& config) {

		_db.defaultAllocator = config.defaultAllocator;
		_db.tempAllocatorSize = config.workerTempAllocatorSize;

		const ThreadCount thread_count = config.threadCount != 0 ? config.threadCount : soul::cast<ThreadCount>(get_hardware_thread_count());

		SOUL_ASSERT(0, thread_count <= Constant::MAX_THREAD_COUNT, "Thread count : %d is more than MAX_THREAD_COUNT : %d", thread_count, Constant::MAX_THREAD_COUNT);
		_db.threadCount = thread_count;

		_db.threadContexts.init(*config.defaultAllocator, thread_count);

		// NOTE(kevinyu): i == 0 is for main thread
		Database::gThreadContext = &_db.threadContexts[0];

		for (uint16 i = 0; i < thread_count; ++i) {
			_db.threadContexts[i].taskCount = 0;
			_db.threadContexts[i].threadIndex = i;
			_db.threadContexts[i].taskDeque.init();
			_db.threadContexts[i].allocatorStack.set_allocator(*config.defaultAllocator);
		}

		_db.isTerminated.store(false, std::memory_order_relaxed);
		for (uint16 i = 1; i < thread_count; ++i) {
			_db.threads[i] = std::thread(&System::_loop, this, &_db.threadContexts[i]);
		}
		_db.activeTaskCount = 0;

		_threadContext().tempAllocator = config.mainThreadTempAllocator;
		
		_initRootTask();
	}

	void System::taskRun(TaskID taskID) {
		Database::gThreadContext->taskDeque.push(taskID);
		{
			std::lock_guard<std::mutex> lock(_db.loopMutex);
			_db.activeTaskCount += 1;
		}
		_db.loopCondVar.notify_all();

	}

	void System::_taskFinish(Task* task) {

		// NOTE(kevinyu): make sure _isCompleteTask() return true only after the task truly finish.
		// Without std::memory_order_release, this operation can be reorder before the task is executed. 
		uint16 unfinishedCount = task->unfinishedCount.fetch_sub(1, std::memory_order_release);

		if (unfinishedCount == 1) {
			// NOTE(kevinyu): This empty lock prevent bug where waitTask() check _isTaskComplete() and then get preempted by OS.
			// For example if waitTask() check _isTaskComplete() and it return false, then before it execute wait() on the condition variable, 
			// it get preempted by OS. In the mean time, another thread can call _finishTask(), without this lock _finishTask() will be able to notify. 
			// When the preempted waitTask() run again, the task is already completed but we still sleep, and because notify
			// has been called on _finishTask, the thread might sleep forever.
			{
				std::lock_guard<std::mutex> lock(_db.waitMutex);
			}
			_db.waitCondVar.notify_all();

			if (task != _taskPtr(TaskID::NULLVAL())) {
				_taskFinish(_taskPtr(task->parentID));
			}
		}
	}

	void System::shutdown() {
		SOUL_ASSERT_MAIN_THREAD();

		SOUL_ASSERT(0, _db.activeTaskCount == 0, "There is still pending task in work deque! Active Task Count = %d.", _db.activeTaskCount);
		_terminate();
		for (uint64 i = 1; i < _db.threadCount; i++) {
			_db.threads[i].join();
		}
		_db.threadContexts.cleanup();
	}

	uint16 System::getThreadCount() const {
		return _db.threadCount;
	}

	uint16 System::getThreadID() const {
		return Database::gThreadContext->threadIndex;
	}

	ThreadContext& System::_threadContext() {
		return _db.threadContexts[getThreadID()];
	}

	void System::pushAllocator(memory::Allocator *allocator) {
		SOUL_ASSERT(0, _db.defaultAllocator != nullptr, "");
		_threadContext().allocatorStack.push_back(allocator);
	}

	void System::popAllocator() {
		SOUL_ASSERT(0, !_threadContext().allocatorStack.empty(), "");
		_threadContext().allocatorStack.pop_back();
	}

	memory::Allocator* System::getContextAllocator() {
		if (_threadContext().allocatorStack.empty()) return _db.defaultAllocator;
		return _threadContext().allocatorStack.back();
	}

	void* System::allocate(uint32 size, uint32 alignment) {
		return getContextAllocator()->allocate(size, alignment);
	}

	void System::deallocate(void* addr, uint32 size) {
		getContextAllocator()->deallocate(addr);
	}

	TempAllocator* System::getTempAllocator() {
		SOUL_ASSERT(0, _threadContext().tempAllocator != nullptr, "");
		return _threadContext().tempAllocator;
	}

}
