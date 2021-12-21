module;
#include <span>
#include <bit>

export module UACEMeshDataCache;

import UACEAllocator;

export namespace UACE::Map
{

	template <UACE::MemManager::Allocator Alloc>
	class StaticMeshCache
	{

	public:
		explicit constexpr StaticMeshCache(Alloc* alloc)
			:alloc(alloc), ptr(UACE::MemManager::Ptr<char, Alloc>(nullptr, nullptr))
		{
			
		}

		[[nodiscard]] constexpr bool prepare(UACE::MemManager::MemSize bytes)
		{
			this->ptr = this->alloc->template createRaw<char>(bytes);
			return this->ptr.allocPtr;
		}

		[[nodiscard]] constexpr bool set(std::span<char> inData)
		{

			if (this->ptr.allocPtr == nullptr ||
				this->ptr.numOfElements < inData.size())
			{
				this->ptr = UACE::MemManager::Ptr<char, Alloc>(nullptr, nullptr);
				this->numOfElements = 0;
				return false;
			}

			std::copy_n(inData.begin(), inData.size(), this->ptr.ptr);
			this->numOfElements = inData.size();

			return true;

		}

		[[nodiscard]] constexpr auto getData() const
		{
			return std::span(this->ptr.ptr, this->numOfElements);
		}

	private:
		Alloc* alloc{ nullptr };
		UACE::MemManager::Ptr<char, Alloc> ptr;

		size_t numOfElements{};

	};

};