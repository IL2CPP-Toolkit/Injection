#include "pch.h"
#include "Snapshot.h"
#include "InjectionHook.h"
#include "GameAssembly.h"
#include "Time.h"
#include "MessageHandler.h"
#include "WindowHelpers.h"

extern "C"
__declspec(dllexport)
void CALLBACK Hook(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
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

	MessageBoxA(NULL, "Hook injected. Click OK to test", "Hook", MB_OK | MB_SYSTEMMODAL);

	HWND hwDispatch = GetMainWindowForProcessId(snapshot.Process().th32ProcessID);
	if (hwDispatch != NULL)
	{
		CallStaticMethodMessage msg{};
		strcpy_s(msg.call.cls.szName, "Time");
		strcpy_s(msg.call.cls.szNamespace, "UnityEngine");
		strcpy_s(msg.call.fn.szName, "set_timeScale");
		msg.call.fn.cParam = 1;
		msg.call.args[0].type = ArgumentValueType::UnsignedLong;
		msg.call.args[0].Number.f = 2.2f;

		COPYDATASTRUCT cds{};
		cds.dwData = static_cast<uint64_t>(InjectedMessageType::CallStaticMethod);
		cds.cbData = sizeof(CallStaticMethodMessage);
		cds.lpData = &msg;
		LRESULT result{ SendMessage(hwDispatch,
			WM_COPYDATA,
			(WPARAM)(HWND)0,
			(LPARAM)(LPVOID)&cds) };

		MessageBoxA(NULL, "Message sent. Click OK to revert.", "Hook", MB_OK | MB_SYSTEMMODAL);
		msg.call.args[0].Number.f = 1.0f;
		result = SendMessage(hwDispatch,
			WM_COPYDATA,
			(WPARAM)(HWND)0,
			(LPARAM)(LPVOID)&cds);
	}
}

