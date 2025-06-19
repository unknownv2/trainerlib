#pragma once

namespace TrainerLib
{
	struct FString
	{
		static bool Initialize();

		wchar_t* Name;
		void* Unknown;

		~FString();
	};
}