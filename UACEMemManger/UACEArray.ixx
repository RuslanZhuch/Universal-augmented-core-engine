module;
#include <cstdlib>
export module UACEArray;

import UACEUnifiedBlockAllocator;

export namespace UACE::Containers
{

	template<typename T, size_t capacity, typename Alloc>
	class Array
	{

	public:
//		explicit Array()
//		{}
		explicit Array(Alloc* alloc) : alloc(alloc),
			dataPtr(alloc->create_raw<T>(capacity))
		{}
		
		Array copy()
		{
			Array<T, capacity, Alloc> newArray(this->alloc);
			newArray.resize(this->currSize);
			memcpy(newArray.data(), this->dataPtr.ptr, this->currSize * sizeof(T));
			return newArray;
		}

		constexpr auto append(const T& data) noexcept
		{
			if ((this->currSize < capacity - 1) && (this->dataPtr.ptr != nullptr))
			{
//				const auto byteOffset{ this->currSize * sizeof(T) };
				memcpy(this->dataPtr.ptr + this->currSize, &data, sizeof(T));
				this->currSize++;
			}
			return this->currSize;
		}

		T& operator[](size_t id) const
		{
//			const auto byteOffset{ id * sizeof(T) };
			auto ptr{ this->dataPtr.ptr + id };
			return *(ptr);
		}

		T* data() noexcept { return this->dataPtr.ptr; }

		void resize(size_t newSize)
		{

			if (newSize > capacity)
				return;

			this->currSize = newSize;

		}

		void clear()
		{

			memset(this->dataPtr.ptr, 0, this->dataPtr.size);

		}
		void reset()
		{

			this->currSize = 0;

		}

		constexpr auto getSize() const noexcept { return this->currSize; }
		constexpr auto getCapacity() const noexcept { return capacity; }

	private:
		Alloc* alloc{ nullptr };
		size_t currSize{ 0 };
		UACE::MemManager::Ptr<T, Alloc> dataPtr;

	};

};