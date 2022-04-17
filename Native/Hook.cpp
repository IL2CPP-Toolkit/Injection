#include "pch.h"
#include "Snapshot.h"
#include "InjectionHook.h"
#include "GameAssembly.h"
#include "Time.h"
#include "MessageHandler.h"
#include "WindowHelpers.h"
#include "Hook.h"
#include <iostream>

extern "C"
__declspec(dllexport)
void CALLBACK Hook(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	AllocConsole();
	FILE* pFile{};
	freopen_s(&pFile, "CONIN$", "r", stdin);
	freopen_s(&pFile, "CONOUT$", "w", stdout);
	freopen_s(&pFile, "CONOUT$", "w", stderr);
	std::string szProcessName{ lpszCmdLine };
	std::wstring wzProcessName{ szProcessName.cbegin(), szProcessName.cend() };
	HMODULE thisModule{};
	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		static_cast<LPCWSTR>(static_cast<void*>(&Hook)), &thisModule) == 0)
	{
		return;
	}
	InjectionHook injection{ thisModule, &HandleHookedMessage };
	Snapshot snapshot;
	if (!snapshot.FindProcess(wzProcessName.c_str()) || !snapshot.FindFirstThread())
		return;
	std::unique_ptr<HookHandle> hook{ injection.Hook(WH_CALLWNDPROC, snapshot.Thread().th32ThreadID) };

	std::cout << "Hook injected!\n";

	HWND hwDispatch = GetMainWindowForProcessId(snapshot.Process().th32ProcessID);
	if (hwDispatch != NULL)
	{
		std::cout << "Setting Timescale = 2.2f\n";
		DispatchSetTimeScale(hwDispatch, 2.2f);
		Sleep(5000);
		std::cout << "Resetting Timescale = 1.0f\n";
		DispatchSetTimeScale(hwDispatch, 1.0f);


	}

	system("pause");
}

void DispatchSetTimeScale(const HWND& hwDispatch, float fTimeScale)
{
	CallStaticMethodMessage msg{};
	strcpy_s(msg.call.cls.szName, "Time");
	strcpy_s(msg.call.cls.szNamespace, "UnityEngine");
	strcpy_s(msg.call.fn.szName, "set_timeScale");
	msg.call.fn.cParam = 1;
	msg.call.args[0].type = ArgumentValueType::Number;
	msg.call.args[0].Number.f = fTimeScale;

	COPYDATASTRUCT cds{};
	cds.dwData = static_cast<uint64_t>(InjectedMessageType::CallStaticMethod);
	cds.cbData = sizeof(CallStaticMethodMessage);
	cds.lpData = &msg;
	LRESULT result{ SendMessage(hwDispatch,
		WM_COPYDATA,
		(WPARAM)(HWND)0,
		(LPARAM)(LPVOID)&cds) };
}

