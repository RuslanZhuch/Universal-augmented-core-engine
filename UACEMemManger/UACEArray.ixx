//module;
//#include <cstdlib>
//#include <cstring>
//export module UACEArray;
//
//import UACEAllocator;
//
//export namespace UACE::Containers
//{
//
//	template<typename T, size_t capacity, UACE::MemManager::Allocator Alloc>
//	class Array
//	{
//
//	public:
//		explicit constexpr Array(Alloc* alloc) : alloc(alloc),
//			dataPtr(alloc->createRaw<T>(capacity))
//		{}
//		
//		[[nodiscard]] constexpr Array copy()
//		{
//			Array<T, capacity, Alloc> newArray(this->alloc);
//			newArray.resize(this->currSize);
//			memcpy(newArray.data(), this->dataPtr.ptr, this->currSize * sizeof(T));
//			return newArray;
//		}
//
//		[[nodiscard]] constexpr auto append(const T& data) noexcept
//		{
//			if ((this->currSize < capacity - 1) && (this->dataPtr.ptr != nullptr))
//			{
////				const auto byteOffset{ this->currSize * sizeof(T) };
//				std::memcpy(this->dataPtr.ptr + this->currSize, &data, sizeof(T));
//				this->currSize++;
//			}
//			return this->currSize;
//		}
//
//		[[nodiscard]] constexpr T& operator[](size_t id) const
//		{
////			const auto byteOffset{ id * sizeof(T) };
//			auto ptr{ this->dataPtr.ptr + id };
//			return *(ptr);
//		}
//
//		[[nodiscard]] constexpr T* data() noexcept { return this->dataPtr.ptr; }
//
//		constexpr void resize(size_t newSize)
//		{
//
//			if (newSize > capacity)
//				return;
//
//			this->currSize = newSize;
//
//		}
//
//		constexpr void clear()
//		{
//
//			memset(this->dataPtr.ptr, 0, this->dataPtr.numOfElements);
//
//		}
//		constexpr void reset()
//		{
//
//			this->currSize = 0;
//
//		}
//
//		[[nodiscard]] constexpr auto getSize() const noexcept { return this->currSize; }
//		[[nodiscard]] constexpr auto getCapacity() const noexcept { return capacity; }
//
//	private:
//		Alloc* alloc{ nullptr };
//		size_t currSize{ 0 };
//		UACE::MemManager::Ptr<T, Alloc> dataPtr;
//
//	};
//
//};