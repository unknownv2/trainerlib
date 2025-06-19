#pragma once

#include <memory>
#include <mutex>
#include <set>
#include "ITaskManager_001.h"
#include "ILogger.h"
#include "Task.h"

namespace TrainerLib
{
	class TaskManager : public ITaskManager_001
	{
		std::mutex _lock;
		std::set<Task*> _tasks;
		std::shared_ptr<ILogger> _logger;

	public:
		std::set<Task*> Tasks;

		explicit TaskManager(std::shared_ptr<ILogger> logger)
			: _logger(move(logger))
		{
			
		}

		// Execute a new task.
		ITask* CreateTask(ITaskRoutine* routine) override;

		// Gracefully end one or more tasks.
		void EndAllTasks() override;

		// Instantly kill one or more tasks.
		void TerminateAllTasks() override;

		virtual ~TaskManager();
	};
}
