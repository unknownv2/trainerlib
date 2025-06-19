#pragma once

#include <Windows.h>
#include "TrainerLib.h"

int Execute(HANDLE hProcess, HMODULE hModule, const init_args& args);
