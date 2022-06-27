//module;
//#include <span>
//#include <bit>
//
//export module UACEMeshDataCache;
//
//import hfog.Core;
//
//export namespace UACE::Map
//{
//
//	template <hfog::CtAllocator Alloc>
//	class StaticMeshCache
//	{
//
//	public:
//		explicit constexpr StaticMeshCache(Alloc* alloc)
//			:alloc(alloc), ptr(UACE::MemManager::Ptr<char, Alloc>(nullptr, nullptr))
//		{
//			
//		}
//
//		[[nodiscard]] constexpr bool prepare(UACE::MemManager::MemSize bytes)
//		{
//			this->ptr = this->alloc->template createRaw<char>(bytes);
//			return this->ptr.allocPtr != nullptr;
//		}
//
//		[[nodiscard]] constexpr bool set(std::span<char> inData)
//		{
//
//			if (this->ptr.allocPtr == nullptr ||
//				this->ptr.numOfElements < inData.size())
//			{
//				this->ptr = UACE::MemManager::Ptr<char, Alloc>(nullptr, nullptr);
//				this->numOfElements = 0;
//				return false;
//			}
//
//			std::copy_n(inData.begin(), inData.size(), this->ptr.ptr);
//			this->numOfElements = inData.size();
//
//			return true;
//
//		}
//
//		[[nodiscard]] constexpr auto getData() const
//		{
//			return std::span(this->ptr.ptr, this->numOfElements);
//		}
//
//	private:
//		Alloc* alloc{ nullptr };
//
//		size_t numOfElements{};
//
//	};
//
//};