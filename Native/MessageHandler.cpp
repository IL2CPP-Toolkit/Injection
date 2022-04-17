#include "pch.h"
#include "GameAssembly.h"
#include "Time.h"
#include "MessageHandler.h"
#include "Il2CppClassHolder.h"
#include "GameAssembly.h"
#include "SystemString.h"
#include <vector>

void CallStaticMethod(GameAssembly& gasm, const CallStaticMethodMessage& msg) noexcept
{
	const CallMethodMessage& call{ msg.call };
	Il2CppClassHolder klass{ GameAssembly::Instance().FindClass(call.cls.szNamespace, call.cls.szName) };
	if (klass.Empty())
		return;

	MethodInfo* pMethodInfo{ klass.FindMethod(call.fn.szName, call.fn.cParam) };
	if (!pMethodInfo)
		return;

	void* pException{};
	void** pArgs{ reinterpret_cast<void**>(malloc(sizeof(uintptr_t) * call.fn.cParam)) };
	if (!pArgs)
		return;

	std::vector<SystemString> strings{};
	for (unsigned int n{ 0 }; n < call.fn.cParam; ++n)
	{
		switch (call.args[n].type)
		{
		case ArgumentValueType::Number:
			pArgs[n] = const_cast<void*>(reinterpret_cast<const void*>(&call.args[n].Number.u64));
			break;
		case ArgumentValueType::String:
			strings.emplace_back(call.args[n].wzString);
			pArgs[n] = &strings.back();
			break;
		}
	}
	gasm.il2cpp_runtime_invoke(
		pMethodInfo,
		nullptr,
		reinterpret_cast<void**>(pArgs),
		&pException);
}

bool HandleCopyData(const COPYDATASTRUCT* lpccds)
{
	static GameAssembly& gasm{ GameAssembly::Initialize(GetModuleHandleA("gameassembly.dll")) };
	
	switch (static_cast<InjectedMessageType>(lpccds->dwData))
	{
	case InjectedMessageType::SetTimeScale: {
		float* pFloat{ reinterpret_cast<float*>(lpccds->lpData) };
		Time::SetTimeScale(gasm, *pFloat);
		return true;
	}
	case InjectedMessageType::CallStaticMethod: {
		CallStaticMethod(gasm, *reinterpret_cast<CallStaticMethodMessage*>(lpccds->lpData));
		return true;
	}
	default:
		break;
	}
	return false;
}

extern "C"
__declspec(dllexport) LRESULT HandleHookedMessage(int code, WPARAM wParam, LPARAM lParam)
{
	const CWPSTRUCT* pMsg{ reinterpret_cast<CWPSTRUCT*>(lParam) };
	if (pMsg)
	{
		switch (pMsg->message) {
		case WM_COPYDATA:
			if (HandleCopyData(reinterpret_cast<COPYDATASTRUCT*>(pMsg->lParam)))
				return CallNextHookEx(NULL, code, wParam, lParam);
				// return 0;
			break;
		default:
			break;
		}
	}

	return CallNextHookEx(NULL, code, wParam, lParam);
}
