module;
#include <array>
#include <ranges>
#include <limits>
#include <algorithm>
#include <mutex>

export module UACEUnifiedBlockAllocator;
export import MemoryManagerCommon;

import UACEDomain;

export namespace UACE::MemManager::UnifiedBlockAllocator
{

	template <typename T, typename Alloc>
	struct Ptr
	{

		size_t size{ 0 };
		T* ptr{ nullptr };
		Alloc* allocPtr{ nullptr };

		T* operator->()
		{
			return ptr;
		}
		T operator*()
		{
			return *ptr;
		}
		
		Ptr() = delete;
		Ptr(T* ptr, Alloc* allocPtr)
			:ptr(ptr), allocPtr(allocPtr), size(sizeof(T) * (allocPtr != nullptr))
		{}
		Ptr(T* ptr, Alloc * allocPtr, size_t size)
			:ptr(ptr), allocPtr(allocPtr), size(size)
		{}


		Ptr(Ptr&& other) noexcept 
			: ptr(std::exchange(other.ptr, nullptr)), 
			allocPtr(std::exchange(other.allocPtr, nullptr)),
			size(std::exchange(other.size, 0))
		{}

		Ptr& operator=(Ptr&& other) noexcept
		{
			std::swap(this->ptr, other.ptr);
			std::swap(this->allocPtr, other.allocPtr);
			std::swap(this->size, other.size);
			return *this;
		}

		~Ptr()
		{
			this->release();
		}

		Ptr(const Ptr& other) = delete;
		Ptr& operator=(const Ptr& other) = delete;

//		auto deepCopy()
//		{
//			auto rawData{ this->allocPtr->create_raw(this->size) };
//			memcpy(rawData, this->ptr, this->size);
//			return Ptr<T>(reinterpret_cast<T*>(rawData), this->size)
//		}

		void release()
		{
			if (this->ptr != nullptr)
			{
				this->allocPtr->dealloc(reinterpret_cast<char*>(ptr), this->size);
				this->ptr = nullptr;
			}
		}

	};

	class UnifiedBlockLogic
	{

	public:
		void dealloc(char* ptr, UACE::MemManager::MemSize size)
		{

			std::lock_guard lg(this->m);

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

		char* getPtr() noexcept { return this->ptr; }

	public:
		void init(char* dataPtr, MemManager::MemSize size)
		{
			this->ptr = dataPtr;
			this->aLens.front() = size;
		}

		char* requestMemory(MemManager::MemSize size) 
		{

			std::lock_guard lg(this->m);

			const auto fLen{ std::ranges::find_if(this->aLens, [this, size](MemManager::MemSize& len)
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

		auto findLowerBound(auto& range, auto val)
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
		auto findUpperBound(auto& range, auto val)
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

		std::mutex m;

	};

	class UnifiedBlockAllocator : public UnifiedBlockLogic
	{

	public:
		UnifiedBlockAllocator(MemManager::MemSize size, char* dataPtr) 
			: size(size)
		{
			this->init(dataPtr, size);
		}

		constexpr auto getIsValid() const { return size > 0; }

		template <typename T, typename ... Args>
		Ptr<T, UnifiedBlockAllocator> create_unique(Args && ... args)
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

		template<typename _T = char>
		Ptr<_T, UnifiedBlockAllocator> create_raw(int numOfEls)
		{
			const auto numOfBytes{ numOfEls * sizeof(_T) };
			char* allowedPtr{ this->requestMemory(numOfBytes) };
			if (allowedPtr == nullptr)
			{
				return Ptr<_T, UnifiedBlockAllocator>(nullptr, nullptr);
			}
			return Ptr<_T, UnifiedBlockAllocator>(reinterpret_cast<_T*>(allowedPtr), this, numOfEls);
		}
		template <typename T, typename ... Args>
		T* create(Args && ... args)
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
		void free(T* ptr)
		{
			const auto sizeToDealloc{ sizeof(T) };
			this->dealloc(reinterpret_cast<char*>(ptr), sizeToDealloc);
		}

		template <typename T>
		Ptr<T, UnifiedBlockAllocator> make_unique(T* ptr)
		{
			return Ptr<T, UnifiedBlockAllocator>(ptr, this);
		}

	private:

	private:
		MemManager::MemSize size{ 0 };

	};

	UnifiedBlockAllocator createAllocator(Domain* domain, MemManager::MemSize size)
	{

		const auto reservedSize{ domain->reserveMemory(size) };
		const auto bReserveSuccess{ reservedSize >= size };
		const auto allocatedSize{ size * bReserveSuccess };

		char* domainPtr{ domain->getMemoryPointer() };
		const auto domainOffset{ domain->getSize() - domain->getFreeSpace() - allocatedSize};

		auto dataPtr{ domainPtr + domainOffset };

		return { allocatedSize, dataPtr };

	}

}