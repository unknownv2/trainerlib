#pragma once

#include <set>
#include <Windows.h>
#include "ITaskManager_001.h"

namespace TrainerLib
{
	class Task : public ITask
	{
		std::set<Task*>& _allTasks;
		ITaskRoutine* _routine;

		HANDLE _hThread = INVALID_HANDLE_VALUE;
		uint32_t _threadId = 0;
		volatile bool _shouldEnd = false;

		static DWORD WINAPI TaskThread(LPVOID lpParam);

		void Kill();

	public:
		Task(std::set<Task*>& allTasks, ITaskRoutine* routine)
			: _allTasks(allTasks), _routine(routine)
		{

		}

		HANDLE Signal();

		void Start();

		void End() override;

		void Terminate() override;

		bool HasEnded() override
		{
			return _hThread == INVALID_HANDLE_VALUE;
		}

		uint32_t ThreadId() override
		{
			return _threadId;
		}

		bool ShouldEnd() override
		{
			return _shouldEnd;
		}

		virtual ~Task()
		{
			Task::Terminate();
		}
	};
}
