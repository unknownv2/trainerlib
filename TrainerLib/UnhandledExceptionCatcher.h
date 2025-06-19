#pragma once

#include <set>
#include <memory>
#include "IHooker.h"
#include "ILogger.h"

namespace TrainerLib
{
	class IErrorHandler
	{
	public:
		virtual void OnError(DWORD threadId, PEXCEPTION_POINTERS exceptionInfo) = 0;
	};

	class UnhandledExceptionCatcher
	{
		struct exception_params
		{
			DWORD threadId;
			PEXCEPTION_POINTERS exceptionInfo;
		};

		static UnhandledExceptionCatcher* _instance;

		static LONG WINAPI UnhandledExceptionFilter(PEXCEPTION_POINTERS ExceptionInfo);
		static LPTOP_LEVEL_EXCEPTION_FILTER WINAPI SetUnhandledExceptionFilterDetour(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
		static DWORD WINAPI HandleException(LPVOID lpExceptionInfo);

		using UnhandledExceptionFilterT = LONG(WINAPI*)(PEXCEPTION_POINTERS);
		using SetUnhandledExceptionFilterT = LPTOP_LEVEL_EXCEPTION_FILTER(WINAPI*)(LPTOP_LEVEL_EXCEPTION_FILTER);

		std::shared_ptr<ILogger> _logger;
		std::shared_ptr<IHooker> _hooker;
		std::set<IErrorHandler*> _handlers;

		IHook* _hook = nullptr;
		SetUnhandledExceptionFilterT _setUnhandledExceptionFilterOrig = nullptr;
		UnhandledExceptionFilterT _previousFilter = nullptr;

		void CreateFilter();

	public:
		UnhandledExceptionCatcher(std::shared_ptr<ILogger> logger, std::shared_ptr<IHooker> hooker)
			: _logger(move(logger)), _hooker(move(hooker))
		{
			CreateFilter();
		}

		void AddErrorHandler(IErrorHandler* handler)
		{
			_handlers.insert(handler);
		}

		void RemoveErrorHandler(IErrorHandler* handler)
		{
			_handlers.erase(handler);
		}

		~UnhandledExceptionCatcher();
	};
}
