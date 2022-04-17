#pragma once

enum class InjectedMessageType : uint8_t
{
	SetTimeScale = 1,
	CallStaticMethod,
};

constexpr size_t MaxNameLength{ 128 };
constexpr size_t MaxArrayLength{ 16 };

union NumberValue {
	float f;
	double d;
	uint64_t u64;
	int64_t i64;
};

struct ClassReference
{
	char szNamespace[MaxNameLength];
	char szName[MaxNameLength];
};

struct MethodReference
{
	char szName[MaxNameLength];
	uint8_t cParam;
};

enum class ArgumentValueType : uint8_t
{
	None = 0,
	UnsignedLong = 1,
	String = 2,
	Array = 3,
};

struct ArrayValue
{
	size_t cbValue;
	uint32_t cValue;
	size_t values[MaxArrayLength];
};

struct ArgumentValue
{
	ArgumentValueType type;
	union {
		NumberValue Number;
		wchar_t wzString[MaxNameLength];
		ArrayValue ValueArray;
	};
};


struct CallMethodMessage
{
	ClassReference cls;
	MethodReference fn;
	ArgumentValue args[MaxArrayLength];
};

struct CallInstanceMethodMessage
{
	void* pInstance;
	CallMethodMessage call;
};

struct CallStaticMethodMessage
{
	CallMethodMessage call;
};

// constexpr size_t sz{ sizeof(CallStaticMethod) };

extern "C"
__declspec(dllexport) LRESULT HandleHookedMessage(int code, WPARAM wParam, LPARAM lParam);