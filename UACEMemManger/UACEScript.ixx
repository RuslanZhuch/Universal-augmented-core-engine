module;
#include <string_view>
export module UACEScript;

import UACEArray;
import UACEScriptLoader;
import UACEScriptDecoder;
import UACEUnifiedBlockAllocator;

template<typename _Alloc>
class Prototype;

template<typename _Alloc, typename _DType>
using ubPtr_t = UACE::MemManager::UnifiedBlockAllocator::Ptr<_DType, _Alloc>;

template<typename _Alloc>
using dataPtr_t = ubPtr_t<_Alloc, UACE::Containers::Array<int, 100, _Alloc>>;

export namespace UACE::Script
{

	template<typename _Alloc>
	class Instance
	{
	public:

		explicit Instance(UACE::Containers::Array<int, 100, _Alloc>* inFuncData,
			UACE::Containers::Array<int, 100, _Alloc>* inStructData, _Alloc* alloc)
			: funcData(dataPtr_t<_Alloc>(nullptr, nullptr)), structData(inStructData), alloc(alloc)
		{
			this->funcData = this->alloc->create_unique<UACE::Containers::Array<int, 100, _Alloc>>(alloc);
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

				if ((point == this->structData->size()) || (point == this->structData->size() - 1))
				{
					break;
				}

				const auto linkPos{ (*this->structData)[point] };
				point = linkPos;

			}
		}

	private:
		Prototype<_Alloc>* proto{ nullptr };

		_Alloc* alloc{ nullptr };

		dataPtr_t<_Alloc> funcData;
		UACE::Containers::Array<int, 100, _Alloc>* structData{ nullptr };
	};

	template<typename _Alloc>
	class Prototype
	{

	public:
		explicit Prototype(_Alloc* alloc)
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
			return this->alloc->create_unique<Instance<_Alloc>>(this->scriptData.funcDataPtr.ptr, this->scriptData.structDataPtr.ptr, this->alloc);
		}

	private:
		
		bool bLoaded{ false };
		_Alloc* alloc{ nullptr };
		UACE::Script::Decoder<_Alloc>::ScriptData scriptData;

	};

};