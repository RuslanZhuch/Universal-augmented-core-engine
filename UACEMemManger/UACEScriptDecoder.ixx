module;
#include <array>
export module UACEScriptDecoder;

import UACEArray;
import UACEUnifiedBlockAllocator;

import UACEBasicScriptFuncs;

template<typename _Alloc, typename _DType>
using ubPtr_t = UACE::MemManager::UnifiedBlockAllocator::Ptr<_DType, _Alloc>;

template<typename _Alloc>
using dataPtr_t = ubPtr_t<_Alloc, UACE::Containers::Array<int, 100, _Alloc>>;

export namespace UACE::Script
{

	template <typename _Alloc>
	class Decoder
	{

	public:

//		template<typename _Alloc>
		struct ScriptData
		{

			explicit ScriptData()
				:funcDataPtr(dataPtr_t<_Alloc>(nullptr, nullptr)), structDataPtr(dataPtr_t<_Alloc>(nullptr, nullptr))
			{}
			ScriptData(auto funcDataPtr, auto structDataPtr)
				:funcDataPtr(std::move(funcDataPtr)), structDataPtr(std::move(structDataPtr))
			{}

			dataPtr_t<_Alloc> funcDataPtr;
			dataPtr_t<_Alloc> structDataPtr;

		};

	public:
		Decoder() = delete;

		explicit Decoder(_Alloc* alloc)
			:alloc(alloc)
		{}

		ScriptData decode(char* rawPtr)
		{

			auto point{ rawPtr };

			int version{ 0 };
			memcpy(&version, point, 4);
			point += 4;

			// Funcion data
			int funcDataLen{ 0 };
			memcpy(&funcDataLen, point, 4);
			point += 4;
			const auto numOfFuncDataBytes{ funcDataLen * 4 };

			auto funcData{ this->alloc->create_unique <
				UACE::Containers::Array<int, 100, _Alloc>>(this->alloc) };
			funcData->resize(funcDataLen);
			memcpy(funcData->data(), point, numOfFuncDataBytes);
			point += numOfFuncDataBytes;

			// Structure data
			int structDataLen{ 0 };
			memcpy(&structDataLen, point, 4);
			point += 4;
			const auto numOfSctructDataBytes{ structDataLen * 4 };

			auto structData{ this->alloc->create_unique <
				UACE::Containers::Array<int, 100, _Alloc>>(this->alloc) };
			structData->resize(structDataLen);
			memcpy(structData->data(), point, numOfSctructDataBytes);
			point += numOfSctructDataBytes;

			// Function names
			int numOfFunctions{ 0 };
			memcpy(&numOfFunctions, point, 4);
			point += 4;

			int currFunctionId{ 0 };
			std::array<char, 32> funcNameBuffer{};
			while (currFunctionId != numOfFunctions)
			{

				memcpy(funcNameBuffer.data(), point, 32);
				point += 32;

				if (std::strcmp(funcNameBuffer.data(), "mathPlus") &&
					std::strcmp(funcNameBuffer.data(), "mathNegate") &&
					std::strcmp(funcNameBuffer.data(), "getObj1") &&
					std::strcmp(funcNameBuffer.data(), "processObj"))
				{
					break;
				}

				int numOfPoints{ 0 };
				memcpy(&numOfPoints, point, 4);
				point += 4;

				int pointId{ 0 };
				while (pointId != numOfPoints)
				{
					int pointPos{ 0 };
					memcpy(&pointPos, point, 4);
					point += 4;

					if (!std::strcmp(funcNameBuffer.data(), "mathPlus"))
					{
						(*funcData.ptr)[pointPos] = (int)UACE::ScriptFuncs::mathPlus;
					}
					else if (!std::strcmp(funcNameBuffer.data(), "mathNegate"))
					{
						(*funcData.ptr)[pointPos] = (int)UACE::ScriptFuncs::mathNegate;
					}
					else if (!std::strcmp(funcNameBuffer.data(), "getObj1"))
					{
						(*funcData.ptr)[pointPos] = (int)UACE::ScriptFuncs::getObj1;
					}
					else if (!std::strcmp(funcNameBuffer.data(), "processObj"))
					{
						(*funcData.ptr)[pointPos] = (int)UACE::ScriptFuncs::processObj;
					}

					pointId++;
				}

				funcNameBuffer.fill(0);

				currFunctionId++;

			}

			return ScriptData(std::move(funcData), std::move(structData));

		}

	private:
		_Alloc* alloc{ nullptr };

	};

}