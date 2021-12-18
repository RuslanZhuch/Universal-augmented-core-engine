module;
#include <array>
#include <limits>
#include <algorithm>

export module UACEUnifiedBlockAllocator;
export import MemoryManagerCommon;
export import UACEAllocator;

import UACEDomain;

export namespace UACE::MemManager::UnifiedBlockAllocator
{

	class UnifiedBlockLogic
	{

	public:
		constexpr void deallocMem(char* ptr, UACE::MemManager::MemSize size)
		{

			const UACE::MemManager::MemSize currOffset{ static_cast<UACE::MemManager::MemSize>(ptr - this->ptr) };
			auto totalSegmentSize{ size };
			auto bNewSegmentCreated{ false };
			const auto fFirstLargerOffset{ this->findLowerBound(this->aPtrOffset, currOffset) };
			if (fFirstLargerOffset != this->aPtrOffset.end())
			{

				const auto offsetId{ std::distance(this->aPtrOffset.begin(), fFirstLargerOffset) };
				const auto len{ *fFirstLargerOffset };
				if (len - currOffset == size)
				{
					totalSegmentSize += this->aLens[offsetId];
				}
				else
				{
					std::shift_right(fFirstLargerOffset, this->aPtrOffset.end(), 1);
					std::shift_right(this->aLens.begin() + offsetId, this->aLens.end(), 1);
				}
				this->aPtrOffset[offsetId] = currOffset;
				this->aLens[offsetId] = totalSegmentSize;
				bNewSegmentCreated = true;

			}
			const auto fFirstSmallerOffset{ this->findUpperBound(this->aPtrOffset, currOffset) };
			if (fFirstSmallerOffset != this->aPtrOffset.end())
			{

				const auto offsetId{ std::distance(this->aPtrOffset.begin(), fFirstSmallerOffset) };
				const auto leftOffset{ *fFirstSmallerOffset };
				if (currOffset - leftOffset == this->aLens[offsetId])
				{
					totalSegmentSize += this->aLens[offsetId];
					this->aLens[offsetId] = totalSegmentSize;
					if (bNewSegmentCreated)
					{
						std::shift_left(fFirstSmallerOffset + 1, this->aPtrOffset.end(), 1);
						std::shift_left(this->aLens.begin() + offsetId + 1, this->aLens.end(), 1);
					}
				}

			}

		}

		void constexpr init(char* dataPtr, MemManager::MemSize size)
		{
			this->ptr = dataPtr;
			this->aLens.front() = size;
		}

		[[nodiscard]] constexpr char* requestMemory(MemManager::MemSize size)
		{

			//Would not compile if requestMemody is constexpr in vs2022 std:c++latest (20.12.2021)
//			const auto fLen{ std::ranges::find_if(this->aLens, [this, size](MemManager::MemSize& len)
//			{
//				return len >= size;
//			}) };
			const auto fLen{ std::find_if(this->aLens.begin(), this->aLens.end(), [this, size](MemManager::MemSize& len)
			{
				return len >= size;
			}) };

			if (fLen == this->aLens.end())
			{
				return nullptr;
			}
			const auto lenId{ std::distance(this->aLens.begin(), fLen) };
			const auto availableLen{ this->aLens[lenId] };
			const auto newAvailableLen{ availableLen - size };
			this->aLens[lenId] = newAvailableLen;

			const auto availableDataPtr{ this->aPtrOffset[lenId] };
			const auto newMemorySlotOffset{ availableDataPtr + size };
			this->aPtrOffset[lenId] = newMemorySlotOffset;

			char* availabelPtr{ this->ptr + availableDataPtr };

			return availabelPtr;

		}

	protected:
		[[nodiscard]] constexpr auto findLowerBound(auto& range, auto val)
		{

			auto currBound{ std::numeric_limits<decltype(val)>::max() };
			auto outIter{ range.begin() };
			for (auto iter{ range.begin() }; auto rVal : range)
			{
				if (rVal > val && rVal < currBound)
				{
					currBound = rVal;
					outIter = iter;
				}
				++iter;
			}
			return outIter;
		}
		[[nodiscard]] constexpr auto findUpperBound(auto& range, auto val)
		{

			auto currBound{ std::numeric_limits<decltype(val)>::min() };
			auto outIter{ range.begin() };
			for (auto iter{ range.begin() }; auto rVal : range)
			{
				if (rVal < val && rVal > currBound)
				{
					currBound = rVal;
					outIter = iter;
				}
				++iter;
			}
			return outIter;
		}

	protected:
		char* ptr{ nullptr };

	private:
		std::array<MemManager::MemSize, 32> aLens{};
		std::array<MemManager::MemSize, 32> aPtrOffset{};

	};

	class UnifiedBlockAllocator : public UACE::MemManager::AllocatorBase<UnifiedBlockAllocator>, 
		private UnifiedBlockLogic
	{

	public:
		constexpr UnifiedBlockAllocator(MemManager::MemSize size, char* dataPtr) 
			: size(size)
		{
			this->init(dataPtr, size);
		}

	private:

		constexpr void dealloc(char* ptr, UACE::MemManager::MemSize size)
		{
			this->deallocMem(ptr, size);
		}

		constexpr auto getIsValidImpl() const { return this->size > 0; }

		template <typename T, typename ... Args>
		[[nodiscard]] constexpr Ptr<T, UnifiedBlockAllocator> createUniqueImpl(Args && ... args)
		{
			const auto needSize{ sizeof(T) };
			char* allowedPtr{ this->requestMemory(needSize) };
			
			if (allowedPtr == nullptr)
			{
				return Ptr<T, UnifiedBlockAllocator>(nullptr, nullptr);
			}
			T* objPtr{ new (allowedPtr) T(std::forward<Args>(args)...) };
			//new (objPtr) T);
			return Ptr<T, UnifiedBlockAllocator>(objPtr, this);
		}

		template<typename T = char>
		[[nodiscard]] constexpr Ptr<T, UnifiedBlockAllocator> createRawImpl(int numOfEls)
		{
			const auto numOfBytes{ numOfEls * sizeof(T) };
			char* allowedPtr{ this->requestMemory(numOfBytes) };
			if (allowedPtr == nullptr)
			{
				return Ptr<T, UnifiedBlockAllocator>(nullptr, nullptr);
			}
			return Ptr<T, UnifiedBlockAllocator>(reinterpret_cast<T*>(allowedPtr), this, numOfEls);
		}
		template <typename T, typename ... Args>
		[[nodiscard]] constexpr T* createImpl(Args && ... args)
		{
			const auto needSize{ sizeof(T) };
			char* allowedPtr{ this->requestMemory(needSize) };
			if (allowedPtr == nullptr)
			{
				return { nullptr };
			}
			T* ptr = reinterpret_cast<T*>(allowedPtr);
			*ptr = T(std::forward<Args>(args)...);
			return ptr;
		}
		template <typename T>
		void constexpr freeImpl(T* ptr, size_t numOfElements)
		{
			const auto sizeToDealloc{ sizeof(T) * numOfElements };
			this->dealloc(reinterpret_cast<char*>(ptr), sizeToDealloc);
		}

		template <typename T>
		[[nodiscard]] constexpr Ptr<T, UnifiedBlockAllocator> makeUniqueImpl(T* ptr)
		{
			return Ptr<T, UnifiedBlockAllocator>(ptr, this);
		}

		[[nodiscard]] constexpr char* getPtrImpl() { return this->ptr; }

	private:
		friend UACE::MemManager::AllocatorBase<UnifiedBlockAllocator>;

	private:
		MemManager::MemSize size{ 0 };

	};

	UnifiedBlockAllocator constexpr createAllocator(Domain* domain, MemManager::MemSize size)
	{

		const auto reservedSize{ domain->reserveMemory(size) };
		const auto bReserveSuccess{ reservedSize >= size };
		const auto allocatedSize{ size * bReserveSuccess };

		char* domainPtr{ domain->getMemoryPointer() };
		const auto domainOffset{ domain->getSize() - domain->getFreeSpace() - allocatedSize};

		auto dataPtr{ domainPtr + domainOffset };

		return { allocatedSize, dataPtr };

	}

};