#include "TaskManager.h"

namespace TrainerLib
{
	ITask* TaskManager::CreateTask(ITaskRoutine* routine)
	{
		auto task = new Task(_tasks, routine);

		_tasks.insert(task);

		task->Start();

		return task;
	}

	void TaskManager::EndAllTasks()
	{
		_lock.lock();

		auto tasks = _tasks;

		if (tasks.empty()) {
			_lock.unlock();
			return;
		}

		auto handles = std::unique_ptr<HANDLE[]>(new HANDLE[tasks.size()]);

		_logger->Log(LogLevel::Debug, "Signalling all %d tasks to end...", static_cast<uint32_t>(tasks.size()));

		auto x = 0;

		for (auto& task : tasks) {
			handles[x++] = task->Signal();
		}

		_lock.unlock();

		WaitForMultipleObjects(x, handles.get(), TRUE, INFINITE);

		_lock.lock();

		_tasks.clear();
			
		_lock.unlock();

		_logger->Log(LogLevel::Debug, "All tasked ended gracefully!");
	}

	void TaskManager::TerminateAllTasks()
	{
		_lock.lock();

		auto tasks = _tasks;

		for (auto& task : tasks) {
			task->Terminate();
		}

		if (!tasks.empty()) {
			_logger->Log(LogLevel::Debug, "Terminated all tasks.");
		}

		_lock.unlock();
	}

	TaskManager::~TaskManager()
	{
		TaskManager::TerminateAllTasks();
	}
}
