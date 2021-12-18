module;
#include <cmath>
#include <type_traits>
export module UACERingBuffer;

export import UACEAllocator;

constexpr size_t HEADER_SIZE{ sizeof(size_t) };

export namespace UACE
{

	template <size_t capacity, UACE::MemManager::Allocator Alloc, typename T = char>
	class RingBuffer
	{

	public:
		explicit RingBuffer(Alloc* alloc)
			:alloc(alloc), 
			dataPtr(alloc->createRaw<T>(capacity)), 
			freespaceLeft(capacity - HEADER_SIZE)
		{
			static_assert(capacity > HEADER_SIZE);
			static_assert(std::is_same_v<T, char>);
			this->writePtr = reinterpret_cast<char*>(this->dataPtr.ptr);
			this->readPtr = reinterpret_cast<char*>(this->dataPtr.ptr);
		}

		[[nodiscard]] T* append(size_t appendSize)
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

			return outPtr;

		}

		[[nodiscard]] size_t copyAndPop(T* destPtr, size_t destSize)
		{

			//Read header
			size_t pkgSize{ 0 };
			memcpy(&pkgSize, this->readPtr, HEADER_SIZE);

			if ((this->readPtr == this->writePtr) || (destSize < pkgSize) || (this->freespaceLeft == capacity))
			{
				return 0;
			}

			this->readPtr = this->movePointer(this->readPtr, HEADER_SIZE);

			auto rootPtr{ this->dataPtr.ptr };

			{
				//First part
				const auto rightSize{ capacity - (this->readPtr - rootPtr) };
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
					const auto rightSize{ capacity - (this->readPtr - rootPtr) };
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
			const auto rightSize{ capacity - (this->writePtr - rootPtr) };

			if (rightSize < needSize)
			{
				this->freespaceLeft -= rightSize;
				this->wrapAddress = this->writePtr;
				this->writePtr = rootPtr;
			}

		}

		[[nodiscard]] char* movePointer(char* currPointer, int steps)
		{
			auto rootPtr{ reinterpret_cast<char*>(this->dataPtr.ptr) };
			const auto absOffset{ currPointer - rootPtr + steps};
			if (absOffset >= capacity)
			{
				const auto rootOffset{ absOffset - capacity };
				return rootPtr + rootOffset;
			}
			else
			{
				return rootPtr + absOffset;
			}
		}

	private:
		MemManager::Ptr<T, Alloc> dataPtr;

		size_t freespaceLeft{};
		Alloc* alloc{ nullptr };

		char* writePtr{ nullptr };
		char* readPtr{ nullptr };

		char* wrapAddress{ nullptr };

	};

}