module;
#include <cstdlib>
export module UACEArray;

import UACEUnifiedBlockAllocator;

export namespace UACE::Containers
{

	template<typename _T, size_t _capacity, typename _Alloc>
	class Array
	{

	public:
//		explicit Array()
//		{}
		explicit Array(_Alloc* alloc) : alloc(alloc),
			dataPtr(alloc->create_raw<_T>(_capacity))
		{}
		
		Array copy()
		{
			Array<_T, _capacity, _Alloc> newArray(this->alloc);
			newArray.resize(this->currSize);
			memcpy(newArray.data(), this->dataPtr.ptr, this->currSize * sizeof(_T));
			return newArray;
		}

		constexpr auto append(const _T& data) noexcept
		{
			if ((this->currSize < _capacity - 1) && (this->dataPtr.ptr != nullptr))
			{
//				const auto byteOffset{ this->currSize * sizeof(_T) };
				memcpy(this->dataPtr.ptr + this->currSize, &data, sizeof(_T));
				this->currSize++;
			}
			return this->currSize;
		}

		_T& operator[](size_t id) const
		{
//			const auto byteOffset{ id * sizeof(_T) };
			auto ptr{ this->dataPtr.ptr + id };
			return *(ptr);
		}

		_T* data() noexcept { return this->dataPtr.ptr; }

		void resize(size_t newSize)
		{

			if (newSize > _capacity)
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

		constexpr auto size() const noexcept { return this->currSize; }
		constexpr auto capacity() const noexcept { return _capacity; }

	private:
		_Alloc* alloc{ nullptr };
		size_t currSize{ 0 };
		UACE::MemManager::UnifiedBlockAllocator::Ptr<_T, _Alloc> dataPtr;

	};

};