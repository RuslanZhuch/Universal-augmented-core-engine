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

	template <typename T, typename Alloc>
	struct Ptr
	{

		size_t numOfElements{ 0 };
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
			:ptr(ptr), allocPtr(allocPtr), numOfElements(1 * (allocPtr != nullptr))
		{}
		Ptr(T* ptr, Alloc* allocPtr, size_t numOfElements)
			:ptr(ptr), allocPtr(allocPtr), numOfElements(numOfElements)
		{}

		Ptr(Ptr&& other) noexcept
			: ptr(std::exchange(other.ptr, nullptr)),
			allocPtr(std::exchange(other.allocPtr, nullptr)),
			numOfElements(std::exchange(other.numOfElements, 0))
		{}

		Ptr& operator=(Ptr&& other) noexcept
		{
			std::swap(this->ptr, other.ptr);
			std::swap(this->allocPtr, other.allocPtr);
			std::swap(this->numOfElements, other.numOfElements);
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
				this->allocPtr->free(this->ptr, this->numOfElements);
				this->ptr = nullptr;
			}
		}

	};

	template <typename Alloc>
	class AllocatorBase
	{

	public:
		AllocatorBase(MemManager::MemSize size, char* dataPtr)
		{

		}

		constexpr auto getIsValid() const { return static_cast<const Alloc*>(this)->getIsValidImpl(); }

		template <typename T, typename ... Args>
		Ptr<T, Alloc> createUnique(Args && ... args)
		{
			return static_cast<Alloc*>(this)->template createUniqueImpl<T>(std::forward<Args>(args)...);
		}

		template<typename T = char>
		Ptr<T, Alloc> createRaw(int numOfEls)
		{
			return static_cast<Alloc*>(this)->createRawImpl<T>(numOfEls);
		}
		template <typename T, typename ... Args>
		T* create(Args && ... args)
		{
			return static_cast<Alloc*>(this)->template createImpl<T>(std::forward<Args>(args)...);
		}
		template <typename T>
		void free(T* ptr, size_t numOfElements = 1)
		{
			static_cast<Alloc*>(this)->freeImpl(ptr, numOfElements);
		}

		template <typename T>
		Ptr<T, Alloc> makeUnique(T* ptr)
		{
			return static_cast<Alloc*>(this)->makeUniqueImpl(ptr);
		}

		char* getPtr()
		{
			return static_cast<Alloc*>(this)->getPtrImpl();
		}

	};

	template<typename T>
	concept Allocator = requires(T && t)
	{
		requires std::derived_from<T, AllocatorBase<T>>;
	};

};