module;
#include <string_view>
export module UACEScript;

import UACEArray;
import UACEScriptLoader;
import UACEScriptDecoder;
import UACEAllocator;

import UACEScriptBasic;

template<typename Alloc>
class Prototype;

export namespace UACE::Script
{

	template<typename Alloc>
	class Instance
	{
	public:

		explicit Instance(UACE::Containers::Array<int, SCRIPT_DATA_BYTES, Alloc>* inFuncData,
			UACE::Containers::Array<int, SCRIPT_DATA_BYTES, Alloc>* inStructData, Alloc* alloc)
			: funcData(dataPtr_t<Alloc>(nullptr, nullptr)), structData(inStructData), alloc(alloc)
		{
			this->funcData = this->alloc->createUnique<UACE::Containers::Array<int, SCRIPT_DATA_BYTES, Alloc>>(alloc);
			*this->funcData.ptr = inFuncData->copy();
			this->funcData.allocPtr = alloc;
		}

		void run()
		{

			int retData{ 0 };

			size_t point{ 0 };
			while (true)
			{

				const auto funcPosition{ (*this->structData)[point++] };
				const auto numOfReturnPointers{ (*this->structData)[point++] };
				const auto numOfLinkPointers{ (*this->structData)[point++] };
				const auto returnPtrsIds{ this->structData->data() + point };
				point += numOfReturnPointers;

				const auto returnsPtr{ &retData };
				const auto numOfParams{ (*this->funcData.ptr)[funcPosition + 1] };
				const auto firstParamOffset{ funcPosition + 2 };
				const auto paramsPtr{ this->funcData->data() + firstParamOffset };

				void (*funcPtr)(char*, char*) = (void(*)(char*, char*))(*this->funcData.ptr)[funcPosition];
				funcPtr((char*)(paramsPtr), (char*)(returnsPtr));

				auto currRetPtrId{ returnPtrsIds };
				for (int retId{ 0 }; retId < numOfReturnPointers; retId++)
				{

					(*this->funcData.ptr)[*currRetPtrId] = retData;
					currRetPtrId++;

				}

				if ((point == this->structData->getSize()) || (point == this->structData->getSize() - 1))
				{
					break;
				}

				const auto linkPos{ (*this->structData)[point] };
				point = linkPos;

			}
		}

	private:
		Prototype<Alloc>* proto{ nullptr };

		Alloc* alloc{ nullptr };

		dataPtr_t<Alloc> funcData;
		UACE::Containers::Array<int, SCRIPT_DATA_BYTES, Alloc>* structData{ nullptr };
	};

	template<UACE::MemManager::Allocator Alloc>
	class Prototype
	{

	public:
		explicit Prototype(Alloc* alloc)
			:alloc(alloc)
		{

		}

		bool load(std::string_view filename)
		{
			this->scriptData = UACE::Script::loadFromFile(filename, this->alloc);
			this->bLoaded = true;
			return this->bLoaded;
		}

		constexpr auto getLoaded() const noexcept { return this->bLoaded; }

		auto createInstance()
		{
			return this->alloc->createUnique<Instance<Alloc>>(this->scriptData.funcDataPtr.ptr, this->scriptData.structDataPtr.ptr, this->alloc);
		}

	private:
		
		bool bLoaded{ false };
		Alloc* alloc{ nullptr };
		UACE::Script::Decoder<Alloc>::ScriptData scriptData;

	};

};