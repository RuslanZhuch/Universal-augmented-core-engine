//module;
//#include <cmath>
//#include <type_traits>
//
//#include <format>
//export module UACERingBuffer;
//
//export import UACEAllocator;
//
//constexpr size_t HEADER_SIZE{ sizeof(size_t) };
//
//export namespace UACE
//{
//
//	template <UACE::MemManager::Allocator Alloc, typename T = char>
//	class RingBuffer
//	{
//
//	public:
//		explicit constexpr RingBuffer(Alloc* alloc, size_t capacity)
//			:alloc(alloc), 
//			dataPtr(alloc->createRaw<T>(capacity)), 
//			freespaceLeft(capacity - HEADER_SIZE),
//			capacity(capacity)
//		{
//			static_assert(std::is_same_v<T, char>);
//			this->writePtr = reinterpret_cast<char*>(this->dataPtr.ptr);
//			this->readPtr = reinterpret_cast<char*>(this->dataPtr.ptr);
//		}
//
//		[[nodiscard]] constexpr T* append(size_t appendSize)
//		{
//
//			const auto bFitForHeader{ (this->freespaceLeft > HEADER_SIZE) };
//			const auto freespaceWithNewHeader{ (this->freespaceLeft - HEADER_SIZE) * bFitForHeader };
//			const auto bHasSpace{ appendSize <= freespaceWithNewHeader };
//			if (!bHasSpace)
//			{
//				return nullptr;
//			}
//
//			this->processWrap(appendSize + HEADER_SIZE);
//
//			auto rootPtr{ reinterpret_cast<char*>(this->dataPtr.ptr) };
//
//			const auto offset{ HEADER_SIZE + appendSize };
//			this->freespaceLeft -= offset;
//
//			memcpy(this->writePtr, &appendSize, HEADER_SIZE);
//
//			auto outPtr{ this->movePointer(this->writePtr, HEADER_SIZE) };
//			this->writePtr = this->movePointer(this->writePtr, offset);
//
//			return outPtr;
//
//		}
//
//		[[nodiscard]] constexpr size_t copyAndPop(T* destPtr, size_t destSize)
//		{
//
//			auto rootPtr{ this->dataPtr.ptr };
//			//Check for wrapping
//			{
//				if (this->readPtr == this->wrapAddress)
//				{
//					const auto rightSize{ this->capacity - (this->readPtr - rootPtr) };
//					this->freespaceLeft += rightSize;
//					this->wrapAddress = nullptr;
//					this->readPtr = rootPtr;
//				}
//			}
//
//			//Read header
//			size_t pkgSize{ 0 };
//			memcpy(&pkgSize, this->readPtr, HEADER_SIZE);
//
//			if ((this->readPtr == this->writePtr) || (destSize < pkgSize) || (this->freespaceLeft == this->capacity))
//			{
//				//system(std::format("echo eq: {}, less {}, free==cap {}\n", this->readPtr == this->writePtr, destSize < pkgSize, this->freespaceLeft == capacity).c_str());
//				return 0;
//			}
//
//			this->readPtr = this->movePointer(this->readPtr, HEADER_SIZE);
//
//
//			{
//				//First part
//				const auto rightSize{ this->capacity - (this->readPtr - rootPtr) };
//				const auto rightSizeToCopy{ std::min(rightSize, pkgSize) };
//				memcpy(destPtr, this->readPtr, rightSizeToCopy);
//				this->readPtr = this->movePointer(this->readPtr, rightSizeToCopy);
//
//				//Second part
//				const auto copySizeLeft{ (pkgSize - rightSizeToCopy) * (pkgSize > rightSizeToCopy) };
//				memcpy(destPtr + rightSizeToCopy, this->readPtr, copySizeLeft);
//				this->readPtr = this->movePointer(this->readPtr, copySizeLeft);
//
//				this->freespaceLeft += pkgSize + HEADER_SIZE;
//			}
//
//			//system(std::format("echo Space left {}\n", this->freespaceLeft).c_str());
//			return pkgSize;
//
//		}
//
//	private:
//		constexpr void processWrap(size_t needSize)
//		{
//
//			auto rootPtr{ this->dataPtr.ptr };
//			const auto rightSize{ this->capacity - (this->writePtr - rootPtr) };
//
//			if (rightSize < needSize)
//			{
//				//system(std::format("echo wrap\n").c_str());
//				this->freespaceLeft -= rightSize;
//				this->wrapAddress = this->writePtr;
//				this->writePtr = rootPtr;
//			}
//
//		}
//
//		[[nodiscard]] constexpr char* movePointer(char* currPointer, size_t steps)
//		{
//			auto rootPtr{ reinterpret_cast<char*>(this->dataPtr.ptr) };
//			const auto absOffset{ currPointer - rootPtr + steps};
//			if (absOffset >= this->capacity)
//			{
//				const auto rootOffset{ absOffset - this->capacity };
//				return rootPtr + rootOffset;
//			}
//			else
//			{
//				return rootPtr + absOffset;
//			}
//		}
//
//	private:
//		MemManager::Ptr<T, Alloc> dataPtr;
//
//		size_t capacity{};
//
//		size_t freespaceLeft{};
//		Alloc* alloc{ nullptr };
//
//		char* writePtr{ nullptr };
//		char* readPtr{ nullptr };
//
//		char* wrapAddress{ nullptr };
//
//	};
//
//}