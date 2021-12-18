module;
#include <cstring>
export module UACEBasicScriptFuncs;

template<typename T>
[[nodiscard]] T extractParam(char* ptr, int offset)
{
	T out;
	memcpy(&out, ptr + offset, sizeof(T));
	return out;
}

template<typename T>
void returnVal(const T& val, char* outPtr, int offset)
{
	memcpy(outPtr + offset, &val, sizeof(T));
}

export namespace UACE::ScriptFuncs
{
	
	struct Obj1
	{
		int data0{ 10 };
		int data1{ 11 };
		int data2{ 12 };
		int data3{ 13 };
	};

	inline Obj1 gObj1{};

	void mathPlus(char* argsPtr, char* retPtr)
	{
		const auto left{ extractParam<int>(argsPtr, 0) };
		const auto right{ extractParam<int>(argsPtr, 4) };
		const auto ans{ left + right };

		returnVal(ans, retPtr, 0);
	};

	void mathNegate(char* argsPtr, char* retPtr)
	{
		const auto val{ extractParam<int>(argsPtr, 0) };

		const auto ans{ -val };

		returnVal(ans, retPtr, 0);
	};

	void getObj1(char* argsPtr, char* retPtr)
	{
		auto objPtr{ &gObj1 };
		returnVal(objPtr, retPtr, 0);
	};

	void processObj(char* argsPtr, char* retPtr)
	{

		const auto summedData{ extractParam<int>(argsPtr, 0) };
		const auto negatedData{ extractParam<int>(argsPtr, 4) };
		const auto obj1Ptr{ extractParam<Obj1*>(argsPtr, 8) };

		obj1Ptr->data1 = summedData;
		obj1Ptr->data2 = negatedData;

	};
};
