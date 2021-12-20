module;
#include <array>
export module UACEScriptDecoder;

import UACEArray;
import UACEAllocator;

import UACEBasicScriptFuncs;

import UACEScriptBasic;

export namespace UACE::Script
{

	template <UACE::MemManager::Allocator Alloc>
	class Decoder
	{

	public:

		struct ScriptData
		{

			explicit constexpr ScriptData()
				:funcDataPtr(dataPtr_t<Alloc>(nullptr, nullptr)), structDataPtr(dataPtr_t<Alloc>(nullptr, nullptr))
			{}
			constexpr ScriptData(auto funcDataPtr, auto structDataPtr)
				:funcDataPtr(std::move(funcDataPtr)), structDataPtr(std::move(structDataPtr))
			{}

			dataPtr_t<Alloc> funcDataPtr;
			dataPtr_t<Alloc> structDataPtr;

		};

	public:
		Decoder() = delete;

		explicit constexpr Decoder(Alloc* alloc)
			:alloc(alloc)
		{}

		[[nodiscard]] constexpr ScriptData decode(char* rawPtr)
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

			auto funcData{ this->alloc->createUnique <
				UACE::Containers::Array<int, 100, Alloc>>(this->alloc) };
			funcData->resize(funcDataLen);
			memcpy(funcData->data(), point, numOfFuncDataBytes);
			point += numOfFuncDataBytes;

			// Structure data
			int structDataLen{ 0 };
			memcpy(&structDataLen, point, 4);
			point += 4;
			const auto numOfSctructDataBytes{ structDataLen * 4 };

			auto structData{ this->alloc->createUnique <
				UACE::Containers::Array<int, 100, Alloc>>(this->alloc) };
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

				if (std::strcmp(funcNameBuffer.data(), "mathPlus") != 0 &&
					std::strcmp(funcNameBuffer.data(), "mathNegate") != 0 &&
					std::strcmp(funcNameBuffer.data(), "getObj1") != 0&&
					std::strcmp(funcNameBuffer.data(), "processObj") != 0)
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
		Alloc* alloc{ nullptr };

	};

}