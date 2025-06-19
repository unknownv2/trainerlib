#include "Task.h"
#include "ThreadProxy.h"

DWORD TrainerLib::Task::TaskThread(LPVOID lpParam)
{
	auto task = static_cast<Task*>(lpParam);

	task->_routine->Execute(task);

	task->Kill();

	return 0;
}

void TrainerLib::Task::Kill()
{
	_hThread = INVALID_HANDLE_VALUE;

	_allTasks.erase(this);

	// No destructor?
	//delete _routine;
}

HANDLE TrainerLib::Task::Signal()
{
	_shouldEnd = true;

	return _hThread;
}

void TrainerLib::Task::Start()
{
	DWORD dwThreadId;
	_hThread = CreateProxyThread(nullptr, 0, TaskThread, this, 0, &dwThreadId);
	_threadId = dwThreadId;
}

void TrainerLib::Task::End()
{
	auto hThread = _hThread;

	_shouldEnd = true;

	WaitForSingleObject(hThread, INFINITE);
}

void TrainerLib::Task::Terminate()
{
	auto hThread = _hThread;

	if (hThread != INVALID_HANDLE_VALUE) {
		TerminateThread(_hThread, 0);
		Kill();
	}
}
