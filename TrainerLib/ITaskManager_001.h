#pragma once

namespace TrainerLib
{
	class ITask
	{
	public:
		virtual void Terminate() = 0;
		virtual void End() = 0;
		virtual bool ShouldEnd() = 0;
		virtual bool HasEnded() = 0;
		virtual uint32_t ThreadId() = 0;
	};

	class ITaskRoutine
	{
	public:
		virtual void Execute(ITask* task) = 0;
	};

	class ITaskManager_001
	{
	public:
		// Execute a new task.
		virtual ITask* CreateTask(ITaskRoutine* routine) = 0;

		// Gracefully end all tasks.
		virtual void EndAllTasks() = 0;

		// Kill all task threads.
		virtual void TerminateAllTasks() = 0;
	};

	static const char* const ITaskManager_001_Version = "ITaskManager_001";
}