#include "pch.h"
#include <unordered_map>
#include "Snapshot.h"
#include "InjectionHook.h"
#include "MessageHandler.h"
#include "WindowHelpers.h"
#include "api.h"

std::unordered_map<DWORD, std::unique_ptr<HookHandle>> g_hookMap;
 
extern "C" __declspec(dllexport) HRESULT WINAPI InjectHook(DWORD procId) noexcept
{
	if (g_hookMap.find(procId) != g_hookMap.cend())
		return E_ILLEGAL_STATE_CHANGE;

	HMODULE thisModule{};
	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		static_cast<LPCWSTR>(static_cast<void*>(&InjectHook)), &thisModule) == 0)
	{
		return E_NOINTERFACE;
	}
	InjectionHook injection{ thisModule, &HandleHookedMessage };
	Snapshot snapshot;
	if (!snapshot.FindProcess(procId) || !snapshot.FindFirstThread())
		return E_INVALIDARG;

	g_hookMap.emplace(procId, injection.Hook(WH_CALLWNDPROC, snapshot.Thread().th32ThreadID));
	return S_OK;
}

extern "C" __declspec(dllexport) HRESULT WINAPI ReleaseHook(DWORD procId) noexcept
{
	const size_t numRemoved{ g_hookMap.erase(procId) };
	if (numRemoved == 0)
		return E_ILLEGAL_STATE_CHANGE;

	return S_OK;
}

extern "C" __declspec(dllexport) InjectResult WINAPI GetHookState(DWORD procId)
{
	if (g_hookMap.find(procId) != g_hookMap.cend())
		return InjectResult::Hooked;

	return InjectResult::Unhooked;
}

extern "C" __declspec(dllexport) bool WINAPI SetTimeScale(DWORD procId, float fTimeScale)
{
	HWND hwndMain{ GetMainWindowForProcessId(procId) };

	COPYDATASTRUCT cds{};
	cds.dwData = static_cast<uint64_t>(InjectedMessageType::SetTimeScale);
	cds.cbData = sizeof(float);
	cds.lpData = &fTimeScale;
	LRESULT result{ SendMessage(hwndMain,
		WM_COPYDATA,
		0,
		reinterpret_cast<LPARAM>(&cds)) };
	return true;
}
