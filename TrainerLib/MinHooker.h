#pragma once

#include <cstdint>
#include <memory>
#include "MinHookLib.h"
#include "IHooker_001.h"
#include "IHooker.h"
#include "ILogger.h"

namespace TrainerLib
{
	class MinHooker : public IHooker
	{
		class Hook : public IHook
		{
			friend class MinHooker;

			MinHooker& _hooker;
			void* _target;
			void* _detour;
			void* _original;

		protected:
			Hook(MinHooker& hooker, void* target, void* detour, void* original)
				: _hooker(hooker), _target(target), _detour(detour), _original(original) { }

		public:
			void* TargetAddress() override
			{
				return _target;
			}

			void* DetourAddress() override
			{
				return _detour;
			}

			void* OriginalAddress() override
			{
				return _original;
			}

			bool Enable() override
			{
				return _hooker.Status(_hooker.InTransaction() ? MH_QueueEnableHook(_target) : MH_EnableHook(_target));
			}

			bool Disable() override
			{
				return _hooker.Status(_hooker.InTransaction() ? MH_QueueDisableHook(_target) : MH_DisableHook(_target));
			}

			void Remove() override
			{
				delete this;
			}

			virtual ~Hook()
			{
				MH_RemoveHook(_target);
			}
		};

		std::shared_ptr<ILogger> _logger;
		uint32_t _transactionLevel = 0;

		bool Status(MH_STATUS status) const;

		bool InTransaction() const
		{
			return _transactionLevel > 0;
		}

	public:
		explicit MinHooker(std::shared_ptr<ILogger> logger)
			: _logger(logger)
		{

		}

		IHook* CreateHook(void* target, void* detour) override;

		void BeginTransaction() override;

		bool CommitTransaction() override;

		~MinHooker() override
		{
			
		}
	};
}
