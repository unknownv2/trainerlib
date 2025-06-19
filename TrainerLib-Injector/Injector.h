#pragma once

#include <Windows.h>
#include <string>

#define PROC_ACCESS PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE

int Inject(HANDLE hProcess, std::wstring dllPath, uint32_t& timeout, HMODULE& hModule);
