module;
#include <utility>
#include <concepts>

export module UACEAllocator;
export import MemoryManagerCommon;

export namespace UACE::MemManager
{

	template <typename Alloc>
	concept AllocatorWeak = requires(Alloc && alloc, 
		MemSize size, char* memBuffer, char* ptr)
	{
		{Alloc(size, memBuffer)};
		{alloc.dealloc(memBuffer, size)} -> std::convertible_to<void>;
		{alloc.free(ptr)} -> std::convertible_to<void>;
		{alloc.getIsValid()} -> std::convertible_to<bool>;
	};

	template <typename T, AllocatorWeak Alloc>
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
			:ptr(ptr), allocPtr(allocPtr), size(sizeof(T)* (allocPtr != nullptr))
		{}
		Ptr(T* ptr, Alloc* allocPtr, size_t size)
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

		void release()
		{
			if (this->ptr != nullptr)
			{
				this->allocPtr->dealloc(reinterpret_cast<char*>(ptr), this->size);
				this->ptr = nullptr;
			}
		}

	};

};