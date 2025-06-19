#pragma once

#include <string>

namespace TrainerLib
{
	struct StringPattern
	{
		std::wstring Start;
		std::wstring End;

		explicit StringPattern(std::wstring pattern)
		{
			auto wildcard = pattern.find('*', 0);

			if (wildcard == std::wstring::npos) {
				Start = pattern;
				return;
			}

			Start = pattern.substr(0, wildcard);
			End = pattern.substr(wildcard + 1);
		}

		bool Matches(std::wstring& str) const
		{
			return str.compare(0, Start.length(), Start) != 0
				&& str.compare(str.length() - End.length(), End.length(), End) != 0;
		}
	};
}