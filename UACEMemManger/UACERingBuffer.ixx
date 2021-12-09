module;
#include <cmath>
#include <type_traits>
export module UACERingBuffer;

export import UACEUnifiedBlockAllocator;

constexpr size_t HEADER_SIZE{ sizeof(size_t) };

export namespace UACE
{

	template <size_t _capacity, typename _Alloc, typename _T = char>
	class RingBuffer
	{

	public:
		explicit RingBuffer(_Alloc* alloc)
			:alloc(alloc), 
			dataPtr(alloc->create_raw<_T>(_capacity)), 
			freespaceLeft(_capacity - HEADER_SIZE)
		{
			static_assert(_capacity > HEADER_SIZE);
			static_assert(std::is_same_v<_T, char>);
			this->writePtr = reinterpret_cast<char*>(this->dataPtr.ptr);
			this->readPtr = reinterpret_cast<char*>(this->dataPtr.ptr);
		}

		_T* append(size_t appendSize)
		{

			const auto bFitForHeader{ (this->freespaceLeft > HEADER_SIZE) };
			const auto freespaceWithNewHeader{ (this->freespaceLeft - HEADER_SIZE) * bFitForHeader };
			const auto bHasSpace{ appendSize <= freespaceWithNewHeader };
			if (!bHasSpace)
			{
				return nullptr;
			}

			this->processWrap(appendSize + HEADER_SIZE);

			auto rootPtr{ reinterpret_cast<char*>(this->dataPtr.ptr) };

			const auto offset{ HEADER_SIZE + appendSize };
			this->freespaceLeft -= offset;

			memcpy(this->writePtr, &appendSize, HEADER_SIZE);

			auto outPtr{ this->movePointer(this->writePtr, HEADER_SIZE) };
			this->writePtr = this->movePointer(this->writePtr, offset);


			//{
			//	const auto rightSize{ _capacity - (this->writePtr - rootPtr) };
			//	if (rightSize < HEADER_SIZE)
			//	{
			//		this->writePtr = this->movePointer(this->writePtr, rightSize);
			//		this->freespaceLeft -= rightSize;
			//	}
			//}

			return outPtr;

		}

		size_t copyAndPop(_T* destPtr, size_t destSize)
		{

			//Read header
			size_t pkgSize{ 0 };
			memcpy(&pkgSize, this->readPtr, HEADER_SIZE);

			if ((this->readPtr == this->writePtr) || (destSize < pkgSize) || (this->freespaceLeft == _capacity))
			{
				return 0;
			}

			this->readPtr = this->movePointer(this->readPtr, HEADER_SIZE);

			auto rootPtr{ this->dataPtr.ptr };

			{
				//First part
				const auto rightSize{ _capacity - (this->readPtr - rootPtr) };
				const auto rightSizeToCopy{ std::min(rightSize, pkgSize) };
				memcpy(destPtr, this->readPtr, rightSizeToCopy);
				this->readPtr = this->movePointer(this->readPtr, rightSizeToCopy);

				//Second part
				const auto copySizeLeft{ (pkgSize - rightSizeToCopy) * (pkgSize > rightSizeToCopy) };
				memcpy(destPtr + rightSizeToCopy, this->readPtr, copySizeLeft);
				this->readPtr = this->movePointer(this->readPtr, copySizeLeft);

				this->freespaceLeft += pkgSize + HEADER_SIZE;
			}

			//Check for wrapping
			{
				if (this->readPtr == this->wrapAddress)
				{
					const auto rightSize{ _capacity - (this->readPtr - rootPtr) };
					this->freespaceLeft += rightSize;
					this->wrapAddress = nullptr;
					this->readPtr = rootPtr;
				}
			}

			return pkgSize;

		}

	private:
		void processWrap(size_t needSize)
		{

			auto rootPtr{ this->dataPtr.ptr };
			const auto rightSize{ _capacity - (this->writePtr - rootPtr) };

			if (rightSize < needSize)
			{
				this->freespaceLeft -= rightSize;
				this->wrapAddress = this->writePtr;
				this->writePtr = rootPtr;
			}

		}

		char* movePointer(char* currPointer, int steps)
		{
			auto rootPtr{ reinterpret_cast<char*>(this->dataPtr.ptr) };
			const auto absOffset{ currPointer - rootPtr + steps};
			if (absOffset >= _capacity)
			{
				const auto rootOffset{ absOffset - _capacity };
				return rootPtr + rootOffset;
			}
			else
			{
				return rootPtr + absOffset;
			}
		}

	private:
		MemManager::UnifiedBlockAllocator::Ptr<_T, _Alloc> dataPtr;

		size_t freespaceLeft{};
		_Alloc* alloc{ nullptr };

		char* writePtr{ nullptr };
		char* readPtr{ nullptr };

		char* wrapAddress{ nullptr };

	};

}